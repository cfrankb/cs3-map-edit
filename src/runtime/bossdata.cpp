/*
    cs3-runtime-sdl
    Copyright (C) 2025  Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//////////////////////////////////////////////////
// Runtime boss data loader implementation.
//
// Mirrors techdocs/bosses/parse.py but runs at load time instead of
// build time: it reads bosses.ini and the JSON hitbox files it points
// at, then populates g_bossData[] / g_hitboxes[].
//
// The parsing rules (composite fields, @seq references, frame indexing,
// grid-snapped hitboxes) are identical to parse.py so the runtime data
// matches what the static generator used to produce.

#include "bossdata.h"

#include "sounds.h"
#include "tilesdata.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include "logger.h"
#include "names.h"

#ifdef USE_QFILE
#include "shared/qtgui/qfilewrap.h"
#else
#include "shared/FileWrap.h"
#endif

// ---------------------------------------------------------------------------
// Minimal JSON reader (object / array / string / number / bool / null).
// The hitbox schema is small and fixed, but a real recursive-descent parser
// keeps whitespace, ordering and nesting correct without hand-rolled scanning.
// ---------------------------------------------------------------------------
namespace
{
    // INI numeric fields go through these instead of bare atoi: garbage
    // ("hp = fast") used to silently become 0 at this mod-data trust
    // boundary. Base 0 accepts the hex forms `type` may use.
    int toInt(const std::string &s, const char *key)
    {
        if (s.empty())
            return 0; // absent field defaults to 0, not garbage
        const char *c = s.c_str();
        char *endp = nullptr;
        long v = std::strtol(c, &endp, 0);
        if (endp == c || *endp != '\0')
        {
            LOGW("Warning: field '%s' value '%s' is not numeric; using 0\n", key, c);
            return 0;
        }
        if (v > INT_MAX)
            return INT_MAX;
        if (v < INT_MIN)
            return INT_MIN;
        return static_cast<int>(v);
    }
    uint8_t toU8(const std::string &s, const char *key)
    {
        if (s.empty())
            return 0;
        const char *c = s.c_str();
        char *endp = nullptr;
        long v = std::strtol(c, &endp, 0);
        if (endp == c || *endp != '\0' || v < 0 || v > 255)
        {
            LOGW("Warning: field '%s' value '%s' outside 0..255; using 0\n", key, c);
            return 0;
        }
        return static_cast<uint8_t>(v);
    }

    struct JsonValue
    {
        enum class Type : uint8_t
        {
            Null,
            Bool,
            Number,
            String,
            Array,
            Object
        };
        Type type = Type::Null;
        bool b = false;
        double num = 0.0;
        std::string str;
        std::vector<JsonValue> arr;
        std::vector<std::pair<std::string, JsonValue>> obj;

        const JsonValue *find(const char *key) const
        {
            for (const auto &kv : obj)
                if (kv.first == key)
                    return &kv.second;
            return nullptr;
        }
        int asInt() const
        {
            // out-of-int-range JSON numbers are UB to cast; saturate instead
            if (num >= static_cast<double>(INT_MAX))
                return INT_MAX;
            if (num <= static_cast<double>(INT_MIN))
                return INT_MIN;
            return static_cast<int>(num);
        }
    };

    class JsonParser
    {
    public:
        bool parse(const char *text, size_t len)
        {
            p = text;
            end = text + len;
            depth_ = 0;
            skipWs();
            if (!parseValue(root_))
                return false;
            skipWs();
            return p == end;
        }
        const JsonValue &root() const { return root_; }

    private:
        const char *p = nullptr;
        const char *end = nullptr;
        int depth_ = 0;
        JsonValue root_;

        void skipWs()
        {
            while (p < end)
            {
                char c = *p;
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                    ++p;
                else
                    break;
            }
        }

        bool expect(char c)
        {
            if (p >= end || *p != c)
                return false;
            ++p;
            return true;
        }

        bool parseValue(JsonValue &out)
        {
            // depth cap: '[[[[...' in a modded file would otherwise
            // overflow the stack via recursive descent (audit Medium)
            if (depth_ >= 64)
                return false;
            ++depth_;
            bool ok = parseValueInner(out);
            --depth_;
            return ok;
        }

        bool parseValueInner(JsonValue &out)
        {
            skipWs();
            if (p >= end)
                return false;
            char c = *p;
            if (c == '{')
                return parseObject(out);
            if (c == '[')
                return parseArray(out);
            if (c == '"')
                return parseString(out);
            if (c == 't' || c == 'f')
                return parseBool(out);
            if (c == 'n')
                return parseNull(out);
            return parseNumber(out);
        }

        bool parseObject(JsonValue &out)
        {
            out.type = JsonValue::Type::Object;
            expect('{');
            skipWs();
            if (p < end && *p == '}')
            {
                ++p;
                return true;
            }
            while (p < end)
            {
                skipWs();
                JsonValue kv;
                if (!parseString(kv))
                    return false;
                skipWs();
                if (!expect(':'))
                    return false;
                JsonValue v;
                if (!parseValue(v))
                    return false;
                for (const auto &prev : out.obj)
                {
                    if (prev.first == kv.str)
                    {
                        LOGW("Warning: duplicate JSON key '%s'; keeping first\n",
                             kv.str.c_str());
                        break;
                    }
                }
                out.obj.emplace_back(std::move(kv.str), std::move(v));
                skipWs();
                if (p < end && *p == ',')
                {
                    ++p;
                    continue;
                }
                if (p < end && *p == '}')
                {
                    ++p;
                    return true;
                }
                return false;
            }
            return false;
        }

        bool parseArray(JsonValue &out)
        {
            out.type = JsonValue::Type::Array;
            expect('[');
            skipWs();
            if (p < end && *p == ']')
            {
                ++p;
                return true;
            }
            while (p < end)
            {
                JsonValue v;
                if (!parseValue(v))
                    return false;
                out.arr.push_back(std::move(v));
                skipWs();
                if (p < end && *p == ',')
                {
                    ++p;
                    continue;
                }
                if (p < end && *p == ']')
                {
                    ++p;
                    return true;
                }
                return false;
            }
            return false;
        }

        bool parseString(JsonValue &out)
        {
            if (!expect('"'))
                return false;
            out.type = JsonValue::Type::String;
            std::string s;
            while (p < end && *p != '"')
            {
                char c = *p++;
                if (c == '\\' && p < end)
                {
                    char e = *p++;
                    switch (e)
                    {
                    case 'n':
                        s += '\n';
                        break;
                    case 't':
                        s += '\t';
                        break;
                    case 'r':
                        s += '\r';
                        break;
                    case '"':
                        s += '"';
                        break;
                    case '\\':
                        s += '\\';
                        break;
                    default:
                        // \uXXXX and any other escape the fixed schema never
                        // uses: keep the raw char (legacy behaviour) but flag it
                        LOGW("Warning: unknown escape '\\%c' in JSON string; keeping raw\n", e);
                        s += e;
                        break;
                    }
                }
                else
                    s += c;
            }
            if (!expect('"'))
                return false;
            out.str = std::move(s);
            return true;
        }

        bool parseBool(JsonValue &out)
        {
            if (p + 4 <= end && std::memcmp(p, "true", 4) == 0)
            {
                p += 4;
                out.type = JsonValue::Type::Bool;
                out.b = true;
                return true;
            }
            if (p + 5 <= end && std::memcmp(p, "false", 5) == 0)
            {
                p += 5;
                out.type = JsonValue::Type::Bool;
                out.b = false;
                return true;
            }
            return false;
        }

        bool parseNull(JsonValue &out)
        {
            if (p + 4 <= end && std::memcmp(p, "null", 4) == 0)
            {
                p += 4;
                out.type = JsonValue::Type::Null;
                return true;
            }
            return false;
        }

        bool parseNumber(JsonValue &out)
        {
            const char *start = p;
            if (p < end && (*p == '-' || *p == '+'))
                ++p;
            while (p < end && (std::isdigit((unsigned char)*p) || *p == '.' || *p == 'e' || *p == 'E' || *p == '-' || *p == '+'))
                ++p;
            if (p == start)
                return false;
            out.type = JsonValue::Type::Number;
            // strtod ignores `end` and needs NUL termination; every caller
            // parses a std::string buffer, so .data()'s NUL bounds the scan.
            out.num = std::strtod(start, nullptr);
            return true;
        }
    };

    // -----------------------------------------------------------------------
    // INI helpers (mirror parse.py tokeniser)
    // -----------------------------------------------------------------------
    std::vector<std::string> tokenize(const std::string &line)
    {
        std::vector<std::string> out;
        size_t i = 0;
        while (i < line.size())
        {
            while (i < line.size() && std::isspace((unsigned char)line[i]))
                ++i;
            if (i >= line.size())
                break;
            size_t start = i;
            while (i < line.size() && !std::isspace((unsigned char)line[i]))
                ++i;
            out.emplace_back(line.substr(start, i - start));
        }
        return out;
    }

    // -----------------------------------------------------------------------
    // Hitbox JSON -> sprite_hitbox_t (mirror read_hitbox_file + handle_hitboxes)
    // -----------------------------------------------------------------------
    constexpr int GRID_SIZE = 8;

    struct HitboxFrame
    {
        int spriteID; // absolute frame id: base_frame + raw_id + sheet*SHEET_SPACER
        std::vector<hitbox_t> boxes;
    };

    bool readFile(const char *path, std::string &text)
    {
#ifdef USE_QFILE
        QFileWrap file;
#else
        CFileWrap file;
#endif
        if (!file.open(path, "rb"))
        {
            LOGE("Error: '%s' not found.\n", path);
            return false;
        }
        long size = file.getSize();
        if (size <= 0)
        {
            file.close();
            LOGE("Error: '%s' is empty.\n", path);
            return false;
        }

        text.resize(static_cast<size_t>(size));
        if (file.read(&text[0], size) != IFILE_OK)
        {
            file.close();
            return false;
        }
        return true;
    }

    bool readHitboxFile(const std::string &path, int grid, int baseFrame,
                        int sheet, std::vector<HitboxFrame> &frames)
    {
        std::string text;
        if (!readFile(path.c_str(), text))
            return false;

        JsonParser parser;
        if (!parser.parse(text.data(), text.size()))
        {
            LOGE("Error: invalid JSON in '%s'.\n", path.c_str());
            return false;
        }

        const JsonValue *frame = parser.root().find("frame");
        const JsonValue *hbs = parser.root().find("hitboxes");
        if (!frame || !hbs || frame->type != JsonValue::Type::Object || hbs->type != JsonValue::Type::Array)
        {
            LOGE("Error: bad schema in '%s'.\n", path.c_str());
            return false;
        }

        int frameWidth = 0;
        int frameHeight = 0;
        int cols = 0;
        auto numAt = [](const JsonValue &obj, const char *key, int &out) -> bool
        {
            const JsonValue *v = obj.find(key);
            if (!v || v->type != JsonValue::Type::Number)
                return false;
            out = v->asInt();
            return true;
        };
        if (!numAt(*frame, "width", frameWidth) ||
            !numAt(*frame, "height", frameHeight) ||
            !numAt(*frame, "cols", cols) ||
            frameWidth <= 0 || frameHeight <= 0 || cols <= 0)
        {
            LOGE("Error: bad frame schema in '%s'.\n", path.c_str());
            return false;
        }

        for (const JsonValue &hb : hbs->arr)
        {
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;
            int type = 0;
            if (hb.type != JsonValue::Type::Object ||
                !numAt(hb, "x", x) || !numAt(hb, "y", y) ||
                !numAt(hb, "w", w) || !numAt(hb, "h", h) ||
                !numAt(hb, "type", type))
            {
                LOGE("Error: bad hitbox entry in '%s'.\n", path.c_str());
                return false;
            }
            if (x < 0 || y < 0 || w < 0 || h < 0)
            { // negative coords would produce negative col/row/frame ids
                LOGE("Error: negative hitbox coord in '%s'.\n", path.c_str());
                return false;
            }
            if (type == 0)
                continue; // ignored frame marker, same as parse.py

            int col = x / frameWidth;
            int row = y / frameHeight;
            int id = row * cols + col;
            int relX = (x - col * frameWidth) / grid;
            int relY = (y - row * frameHeight) / grid;

            hitbox_t box;
            box.x = relX;
            box.y = relY;
            box.width = w / grid;
            box.height = h / grid;
            box.type = type;

            // Absolute frame id, keyed exactly like parse.py's all_frames.
            int frame_id = baseFrame + id + sheet * BossData::SHEET_SPACER;

            // Insert-or-append keyed by absolute frame id (dict preserves
            // first-seen order).
            HitboxFrame *target = nullptr;
            for (auto &fr : frames)
            {
                if (fr.spriteID == frame_id)
                {
                    target = &fr;
                    break;
                }
            }
            if (!target)
            {
                frames.push_back(HitboxFrame{frame_id, {}});
                target = &frames.back();
            }
            target->boxes.push_back(box);
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Boss accumulator (mirror parse.py seq dict)
    // -----------------------------------------------------------------------
    struct BossAccum
    {
        std::string name;
        std::map<std::string, std::string> fields; // final-name -> raw token

        bossData_t build(int sheetIndex) const
        {
            bossData_t d{};
            d.sheet = sheetIndex;
            d.name = name.c_str(); // points into the pooled names vector

            auto get = [this](const char *k) -> std::string
            {
                auto it = fields.find(k);
                return it == fields.end() ? std::string() : it->second;
            };

            d.speed = toInt(get("speed"), "speed");
            d.speed_anime = toInt(get("speed_anime"), "speed_anime");
            d.hp = toInt(get("hp"), "hp");
            d.type = toU8(get("type"), "type");
            d.score = toInt(get("score"), "score");
            d.damage = toInt(get("damage"), "damage");
            d.damage_attack = toInt(get("damage_attack"), "damage_attack");
            d.damage_special1 = toInt(get("damage_special1"), "damage_special1");
            d.damage_special2 = toInt(get("damage_special2"), "damage_special2");
            d.flags = resolveFlags(get("flags"));
            d.path = resolvePath(get("path"));
            d.bullet = resolveTile(get("bullet"));
            d.bullet_rate = toU8(get("bullet_rate"), "bullet_rate");
            d.bullet_algo = resolvePath(get("bullet_algo"));
            d.bullet_sound = resolveSound(get("bullet_sound"));
            {
                std::string ttl = get("bullet_ttl");
                d.bullet_ttl = (ttl == "NoTTL") ? BossData::NoTTL : toInt(ttl, "bullet_ttl");
            }
            d.attack_sound = resolveSound(get("attack_sound"));
            d.distance_chase = toInt(get("distance_chase"), "distance_chase");
            d.distance_pursuit = toInt(get("distance_pursuit"), "distance_pursuit");
            d.distance_attack = toInt(get("distance_attack"), "distance_attack");
            std::string goal = get("is_goal");
            d.is_goal = (goal == "true" || goal == "1");
            std::string show = get("show_details");
            d.show_details = (show == "true" || show == "1");
            d.color_hp = resolveColor(get("color_hp"));
            d.color_name = resolveColor(get("color_name"));
            d.aims = toInt(get("aims"), "aims");
            if (d.aims <= 0)
            {
                // aim count feeds the frame layout and CBoss::currentFrame()
                // modulo: flag garbage/missing keys at load instead of
                // silently dividing by zero later.
                LOGW("Warning: boss '%s' has garbage/missing aims (%d); using 1\n",
                     name.c_str(), d.aims);
                d.aims = 1;
            }

            auto seqFrom = [&](const char *name) -> boss_seq_t
            {
                boss_seq_t s{};
                auto bit = fields.find(std::string(name) + "_base");
                auto lit = fields.find(std::string(name) + "_len");
                s.base = bit == fields.end() ? 0 : std::atoi(bit->second.c_str());
                s.lenght = lit == fields.end() ? 0 : std::atoi(lit->second.c_str());
                return s;
            };

            d.moving = seqFrom("moving");
            d.attack = seqFrom("attack");
            d.hurt = seqFrom("hurt");
            d.death = seqFrom("death");
            d.idle = seqFrom("idle");
            if (fields.count("hitbox"))
            {
                // "x, y, w, h , 0" (parse.py appends ", 0" for the type slot)
                const std::string &hbStr = fields.at("hitbox");
                std::vector<int> v;
                size_t i = 0;
                while (i < hbStr.size())
                {
                    while (i < hbStr.size() &&
                           !std::isdigit((unsigned char)hbStr[i]) &&
                           hbStr[i] != '-')
                        ++i;
                    size_t start = i;
                    while (i < hbStr.size() &&
                           (std::isdigit((unsigned char)hbStr[i]) || hbStr[i] == '-'))
                        ++i;
                    if (i > start)
                        v.push_back(std::atoi(hbStr.substr(start, i - start).c_str()));
                }
                d.hitbox.x = v.size() > 0 ? v[0] : 0;
                d.hitbox.y = v.size() > 1 ? v[1] : 0;
                d.hitbox.width = v.size() > 2 ? v[2] : 0;
                d.hitbox.height = v.size() > 3 ? v[3] : 0;
                d.hitbox.type = v.size() > 4 ? v[4] : 0;
            }

            return d;
        }
    };

    // -----------------------------------------------------------------------
    // ini parser (mirror the parse.py main loop)
    // -----------------------------------------------------------------------
    struct Loader
    {
        std::vector<bossData_t> bosses;
        std::vector<std::string> namePool;
        std::vector<HitboxFrame> hitboxFrames;

        int sheet = 0;
        int frame = 0;
        int lineNo = 0;
        std::string baseDir; // hitbox files resolve relative to the ini dir (parse.py behaviour)

        void pushBoss(BossAccum &seq)
        {
            if (!seq.fields.empty() || !seq.name.empty())
            {
                bosses.push_back(seq.build(sheet));
                namePool.push_back(seq.name);
            }
            seq = BossAccum{};
        }

        int load(const char *iniDir, const char *hitboxDir)
        {
            (void)hitboxDir; // kept for API symmetry; hitbox files resolve off baseDir
            baseDir = iniDir;
            const std::string iniPath = std::string(iniDir) + "/bosses.ini";
            std::string text;
            if (!readFile(iniPath.c_str(), text))
                return 1;

            BossAccum seq;
            bool hasSection = false;
            std::string section;

            auto flush = [&]()
            {
                if (hasSection)
                    pushBoss(seq); // pushBoss resets seq
                hasSection = false;
            };

            for (const std::string &rawLine : splitLines(text))
            {
                ++lineNo;
                std::string t = rawLine;
                // strip trailing comment
                size_t hash = t.find('#');
                if (hash != std::string::npos)
                    t = t.substr(0, hash);
                // strip trailing CR (Windows line endings)
                if (!t.empty() && t.back() == '\r')
                    t.pop_back();
                // trim
                size_t b = t.find_first_not_of(" \t");
                size_t eidx = t.find_last_not_of(" \t");
                if (b == std::string::npos)
                    continue;
                t = t.substr(b, eidx - b + 1);
                if (t.empty())
                    continue;

                std::vector<std::string> e = tokenize(t);

                // section header
                if (t.front() == '[')
                {
                    flush();
                    std::string sec = t;
                    sec.erase(sec.begin()); // '[' guaranteed by the if above
                    if (!sec.empty() && sec.back() == ']')
                        sec.pop_back();
                    if (sec.empty())
                    {
                        LOGE("Error: empty section header on line %d.\n", lineNo);
                        return 1;
                    }
                    if (std::find(namePool.begin(), namePool.end(), sec) != namePool.end())
                        LOGW("Warning: duplicate boss section '[%s]' on line %d; getBossData will use the first\n",
                             sec.c_str(), lineNo);
                    section = sec;
                    seq.name = section;
                    hasSection = true;
                    continue;
                }

                // >>>sheet N
                if (e[0] == ">>>sheet" && e.size() == 2)
                {
                    sheet = std::atoi(e[1].c_str());
                    frame = 0;
                    continue;
                }

                if (!hasSection)
                {
                    // general config (private/define lines): parsed and
                    // discarded — nothing in the runtime consumed them
                    continue;
                }

                // boss config
                const std::string &item = e[0];

                // composite fields: speed, damage, damage_*, distance_*, color_*, bullet_*
                if (item == "speed" || item == "damage" || item == "distance" ||
                    item == "color" || item == "bullet")
                {
                    for (size_t i = 1; i < e.size(); ++i)
                    {
                        size_t eq = e[i].find('=');
                        if (eq != std::string::npos)
                        {
                            if (eq == 0)
                            {
                                LOGW("Warning: empty composite key in '%s' on line %d: %s\n",
                                     item.c_str(), lineNo, e[i].c_str());
                                continue;
                            }
                            std::string key = item + "_" + e[i].substr(0, eq);
                            std::string val = e[i].substr(eq + 1);
                            seq.fields[key] = val;
                        }
                        else
                        {
                            seq.fields[item] = e[i];
                        }
                    }
                    continue;
                }

                if (item == "hitbox")
                {
                    if (e.size() == 5)
                        seq.fields["hitbox"] = joinSpace(e, 1) + " , 0";
                    else
                        LOGW("Warning: hitbox on line %d must have 4 dimensions: %s\n", lineNo, t.c_str());
                    continue;
                }

                if (item == "flags")
                {
                    seq.fields["flags"] = joinSpace(e, 1);
                    continue;
                }

                // scalar attributes
                auto setScalar = [&](const std::string &k, const std::string &v)
                {
                    seq.fields[k] = v;
                };

                if (item == "type" || item == "hp" || item == "score" ||
                    item == "path" || item == "attack_sound" || item == "is_goal" ||
                    item == "show_details" || item == "aims")
                {
                    if (e.size() < 2)
                    {
                        LOGW("Warning: '%s' on line %d is missing a value: %s\n",
                             item.c_str(), lineNo, t.c_str());
                        continue;
                    }
                    setScalar(item, e[1]);
                }
                else if (item == "moving" || item == "attack" || item == "hurt" ||
                         item == "death" || item == "idle")
                {
                    handleSeq(seq, item, e);
                }
                else
                {
                    LOGW("Warning: unknown field '%s' on line %d: %s\n", item.c_str(), lineNo, t.c_str());
                }
            }
            flush();
            return 0;
        }

        // Build a boss_seq_t for an animation line, advancing the frame counter.
        void handleSeq(BossAccum &seq, const std::string &seqName,
                       const std::vector<std::string> &e)
        {
            if (e.size() < 2)
            {
                LOGW("Warning: %s line is missing a frame count.\n", seqName.c_str());
                return;
            }
            std::string hitboxFile = e.size() == 3 ? e[2] : "";

            if (e[1].front() == '@')
            {
                // reference to another sequence already defined on this boss
                std::string ref = e[1].substr(1);
                auto it = seq.fields.find(ref + "_base");
                if (it == seq.fields.end())
                {
                    LOGW("Warning: '%s' on line %d references undefined sequence '%s'\n",
                         seqName.c_str(), lineNo, ref.c_str());
                    return;
                }
                if (!hitboxFile.empty())
                    LOGW("Warning: hitbox file `%s` on line %d ignored for @ref sequence\n",
                         hitboxFile.c_str(), lineNo); // parse.py ignores it too
                boss_seq_t s{};
                s.base = std::atoi(it->second.c_str());
                auto lit = seq.fields.find(ref + "_len");
                s.lenght = lit == seq.fields.end() ? 0 : std::atoi(lit->second.c_str());
                seq.fields[seqName + "_base"] = std::to_string(s.base);
                seq.fields[seqName + "_len"] = std::to_string(s.lenght);
                return;
            }

            if (!hitboxFile.empty())
            {
                std::string full = baseDir + "/" + hitboxFile;
                // failure must NOT skip base/len below or every later
                // sequence's frames shift (audit Medium); just lose the boxes
                if (!readHitboxFile(full, GRID_SIZE, frame, sheet, hitboxFrames))
                    LOGW("Warning: continuing '%s' on line %d without hitbox file '%s'\n",
                         seqName.c_str(), lineNo, hitboxFile.c_str());
            }

            int count = toInt(e[1], seqName.c_str());
            if (count < 0)
            {
                LOGW("Warning: negative frame count on line %d (%d); using 0\n",
                     lineNo, count);
                count = 0;
            }
            seq.fields[seqName + "_base"] = std::to_string(frame);
            seq.fields[seqName + "_len"] = std::to_string(count);

            int aims = seq.fields.count("aims") ? std::atoi(seq.fields.at("aims").c_str()) : 1;
            if (aims <= 0)
            {
                LOGW("Warning: sequence '%s' of boss '%s' has garbage/missing aims (%d); using 1\n",
                     seqName.c_str(), seq.name.c_str(), aims);
                aims = 1;
            }
            frame += count * aims;
        }

        // ---- small string helpers ----
        static std::vector<std::string> splitLines(const std::string &s)
        {
            std::vector<std::string> out;
            std::string cur;
            for (char c : s)
            {
                if (c == '\n')
                {
                    out.push_back(cur);
                    cur.clear();
                }
                else
                    cur += c;
            }
            out.push_back(cur);
            return out;
        }
        static std::string joinSpace(const std::vector<std::string> &v, size_t from)
        {
            std::string out;
            for (size_t i = from; i < v.size(); ++i)
            {
                if (i > from)
                    out += ' ';
                out += v[i];
            }
            return out;
        }
    };
} // namespace

// ---------------------------------------------------------------------------
// globals + loader entry points
// ---------------------------------------------------------------------------
int g_bossCount = 0;
int g_hitboxCount = 0;
const bossData_t *g_bossData = nullptr;
const sprite_hitbox_t *g_hitboxes = nullptr;

// Storage kept alive until freeBossData() so the returned pointers
// (g_bossData, g_hitboxes, and every d.name) stay valid; the statics' own
// destructors are the last-resort release if freeBossData() is never called.
static std::vector<bossData_t> g_pooledBosses;
static std::vector<std::string> g_pooledNames;
static std::vector<sprite_hitbox_t> g_pooledHitboxes;

int loadBossData(const std::string &iniDir, const std::string &hitboxDir)
{
    freeBossData();

    Loader loader;
    int rc = loader.load(iniDir.c_str(), hitboxDir.c_str());
    if (rc != 0)
        return rc;

    if (loader.bosses.empty())
    {
        LOGE("Error: no bosses parsed.\n");
        return 1;
    }

    // Build sprite_hitbox_t array in first-seen sprite-id order.
    std::vector<sprite_hitbox_t> hb;
    hb.reserve(loader.hitboxFrames.size());
    for (auto &fr : loader.hitboxFrames)
    {
        sprite_hitbox_t s{};
        s.spriteID = fr.spriteID;
        // only MAX_HITBOX_PER_FRAME boxes fit in sprite_hitbox_t; recording
        // more would make the consumer (CBoss::draw) read past hitboxes[]
        if (fr.boxes.size() > MAX_HITBOX_PER_FRAME)
            LOGW("Warning: frame %d has %zu hitboxes; keeping %d.\n",
                 fr.spriteID, fr.boxes.size(), MAX_HITBOX_PER_FRAME);
        s.count = static_cast<int>(std::min(fr.boxes.size(),
                                            static_cast<size_t>(MAX_HITBOX_PER_FRAME)));
        for (size_t i = 0; i < fr.boxes.size() && i < MAX_HITBOX_PER_FRAME; ++i)
            s.hitboxes[i] = fr.boxes[i];
        hb.push_back(s);
    }

    // Adopt the loader's storage into the static pools.
    g_pooledBosses = std::move(loader.bosses);
    g_pooledNames = std::move(loader.namePool);
    g_pooledHitboxes = std::move(hb);

    // Re-point each boss's name into the pooled names array (build() pointed
    // into the transient BossAccum before the move). bosses and names are
    // only ever grown together in pushBoss(), so index i pairs with name i —
    // keep that invariant if pushBoss ever grows a branch.
    for (size_t i = 0; i < g_pooledBosses.size(); ++i)
        g_pooledBosses[i].name = g_pooledNames.at(i).c_str();

    g_bossData = g_pooledBosses.data();
    g_bossCount = static_cast<int>(g_pooledBosses.size());
    g_hitboxes = g_pooledHitboxes.data();
    g_hitboxCount = static_cast<int>(g_pooledHitboxes.size());

    return 0;
}

void freeBossData()
{
    g_pooledBosses.clear();
    g_pooledNames.clear();
    g_pooledHitboxes.clear();
    g_bossData = nullptr;
    g_hitboxes = nullptr;
    g_bossCount = 0;
    g_hitboxCount = 0;
}
