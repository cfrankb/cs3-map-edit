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
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <utility>

#include "logger.h"
#include "shared/FileWrap.h"

#define _N(__VAL) {#__VAL, __VAL}

#ifdef USE_QFILE
#include "shared/qtgui/qfilewrap.h"
#endif

// ---------------------------------------------------------------------------
// Minimal JSON reader (object / array / string / number / bool / null).
// The hitbox schema is small and fixed, but a real recursive-descent parser
// keeps whitespace, ordering and nesting correct without hand-rolled scanning.
// ---------------------------------------------------------------------------
namespace
{
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
        int asInt() const { return static_cast<int>(num); }
    };

    class JsonParser
    {
    public:
        bool parse(const char *text, size_t len)
        {
            p = text;
            end = text + len;
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
            out.num = std::strtod(start, nullptr);
            return true;
        }
    };

    // -----------------------------------------------------------------------
    // Name -> value resolvers. The static generator emitted enum names and
    // let the compiler resolve them; here we resolve to the same values.
    // -----------------------------------------------------------------------
    struct NameValue
    {
        const char *name;
        uint32_t value;
    };

    const NameValue kColors[] = {
        _N(CLEAR),
        _N(ALPHA),
        _N(WHITE),
        _N(YELLOW),
        _N(PURPLE),
        _N(DARKPURPLE),
        _N(BLACK),
        _N(GREEN),
        _N(DARKGREEN),
        _N(LIME),
        _N(BLUE),
        _N(MIDBLUE),
        _N(CYAN),
        _N(RED),
        _N(DARKRED),
        _N(DARKBLUE),
        _N(DARKGRAY),
        _N(GRAY),
        _N(LIGHTGRAY),
        _N(LIGHTSLATEGRAY),
        _N(ORANGE),
        _N(DARKORANGE),
        _N(CORAL),
        _N(PINK),
        _N(HOTPINK),
        _N(DEEPPINK),
        _N(OLIVE),
        _N(MEDIUMSEAGREEN),
        _N(SEAGREEN),
        _N(BLUEVIOLET),
        _N(DEEPSKYBLUE),
        _N(LAVENDER),
        _N(DARKSLATEGREY),
        _N(BROWN),
        _N(RED_BROWN),
        _N(PALE_BROWN),
        _N(DARK_BROWN),
        _N(DARK_BROWN2),
    };

    const NameValue kSounds[] = {
        _N(SOUND_NONE),
        _N(SOUND_GRUUP),
        _N(SOUND_KEY),
        _N(SOUND_0009),
        _N(SOUND_COIN1),
        _N(SOUND_OUCHFAST),
        _N(SOUND_POUF),
        _N(SOUND_POW),
        _N(SOUND_POWERUP),
        _N(SOUND_POWERUP2),
        _N(SOUND_POWERUP3),
        _N(SOUND_BONUS),
        _N(SOUND_BEEP),
        _N(SOUND_GOLD3),
        _N(SOUND_SPRING05),
        _N(SOUND_EXPLOSION1),
        _N(SOUND_EXIT),
        _N(SOUND_DEATH),
        _N(SOUND_BELLS1),
        _N(SOUND_HIT2),
        _N(SOUND_SHOOT3),
        _N(SOUND_SPLASH01),
        _N(SOUND_DINOSAUR3),
    };

    const NameValue kPaths[] = {
        {"NONE", BossData::NONE},
        {"ASTAR", BossData::ASTAR},
        {"BFS", BossData::BFS},
        {"LOS", BossData::LOS},
        {"ASTAR_SMOOTH", BossData::ASTAR_SMOOTH},
    };

    const NameValue kTiles[] = {
        _N(TILES_BLANK),
        _N(TILES_FIREBALL),
        _N(TILES_BULLETY1),
        _N(TILES_FIREBALL_SM),
    };

    // Boss flags are #defines in the data, so their values are baked in here.
    const NameValue kFlags[] = {
        _N(FLAG_FIREBALL),
        _N(BOSS_FLAG_PROXIMITY_ATTACK),
        _N(BOSS_FLAG_ICE_DAMAGE),
    };

    const NameValue *lookup(const NameValue *table, size_t n, const std::string &name)
    {
        for (size_t i = 0; i < n; ++i)
            if (table[i].name == name)
                return &table[i];
        return nullptr;
    }

    // "Path::ASTAR" or bare "ASTAR"
    uint32_t resolvePath(const std::string &s)
    {
        std::string name = s;
        const std::string prefix = "Path::";
        if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0)
            name = name.substr(prefix.size());
        const NameValue *nv = lookup(kPaths, sizeof(kPaths) / sizeof(kPaths[0]), name);
        if (!nv)
            LOGW("failed resolvePath: %s", s.c_str());
        return nv ? nv->value : 0;
    }

    uint8_t resolveTile(const std::string &s)
    {
        const NameValue *nv = lookup(kTiles, sizeof(kTiles) / sizeof(kTiles[0]), s);
        if (!nv)
            LOGW("failed resolveTile: %s", s.c_str());
        return nv ? static_cast<uint8_t>(nv->value) : 0;
    }

    uint8_t resolveSound(const std::string &s)
    {
        const NameValue *nv = lookup(kSounds, sizeof(kSounds) / sizeof(kSounds[0]), s);
        if (!nv)
            LOGW("failed resolveSound: %s", s.c_str());
        return nv ? static_cast<uint8_t>(nv->value) : 0;
    }

    Color resolveColor(const std::string &s)
    {
        const NameValue *nv = lookup(kColors, sizeof(kColors) / sizeof(kColors[0]), s);
        if (!nv)
            LOGW("failed resolveColor: %s", s.c_str());
        return nv ? static_cast<Color>(nv->value) : CLEAR;
    }

    // OR-combines one or more named flags; "0" or empty yields 0.
    uint32_t resolveFlags(const std::string &s)
    {
        uint32_t acc = 0;
        std::string tokens = s;
        size_t i = 0;
        while (i < tokens.size())
        {
            while (i < tokens.size() && std::isspace((unsigned char)tokens[i]))
                ++i;
            if (i >= tokens.size())
                break;
            size_t start = i;
            while (i < tokens.size() && !std::isspace((unsigned char)tokens[i]))
                ++i;
            std::string name = tokens.substr(start, i - start);
            if (name.empty())
                continue;
            const NameValue *nv = lookup(kFlags, sizeof(kFlags) / sizeof(kFlags[0]), name);
            if (nv)
                acc |= nv->value;
            else if (name == "0")
                ; // explicit none
            else
                acc |= static_cast<uint32_t>(std::strtoul(name.c_str(), nullptr, 0));
        }
        return acc;
    }

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

        int frameWidth = frame->find("width")->asInt();
        int frameHeight = frame->find("height")->asInt();
        int cols = frame->find("cols")->asInt();

        for (const JsonValue &hb : hbs->arr)
        {
            int x = hb.find("x")->asInt();
            int y = hb.find("y")->asInt();
            int w = hb.find("w")->asInt();
            int h = hb.find("h")->asInt();
            int type = hb.find("type")->asInt();
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
        int sheet = 0;
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

            d.speed = std::atoi(get("speed").c_str());
            d.speed_anime = std::atoi(get("speed_anime").c_str());
            d.hp = std::atoi(get("hp").c_str());
            d.type = static_cast<uint8_t>(std::strtoul(get("type").c_str(), nullptr, 0));
            d.score = std::atoi(get("score").c_str());
            d.damage = std::atoi(get("damage").c_str());
            d.damage_attack = std::atoi(get("damage_attack").c_str());
            d.damage_special1 = std::atoi(get("damage_special1").c_str());
            d.damage_special2 = std::atoi(get("damage_special2").c_str());
            d.flags = resolveFlags(get("flags"));
            d.path = resolvePath(get("path"));
            d.bullet = resolveTile(get("bullet"));
            d.bullet_rate = static_cast<uint8_t>(std::atoi(get("bullet_rate").c_str()));
            d.bullet_algo = resolvePath(get("bullet_algo"));
            d.bullet_sound = resolveSound(get("bullet_sound"));
            {
                std::string ttl = get("bullet_ttl");
                d.bullet_ttl = (ttl == "NoTTL") ? BossData::NoTTL : std::atoi(ttl.c_str());
            }
            d.attack_sound = resolveSound(get("attack_sound"));
            d.distance_chase = std::atoi(get("distance_chase").c_str());
            d.distance_pursuit = std::atoi(get("distance_pursuit").c_str());
            d.distance_attack = std::atoi(get("distance_attack").c_str());
            std::string goal = get("is_goal");
            d.is_goal = (goal == "true" || goal == "1");
            std::string show = get("show_details");
            d.show_details = (show == "true" || show == "1");
            d.color_hp = resolveColor(get("color_hp"));
            d.color_name = resolveColor(get("color_name"));
            d.aims = std::atoi(get("aims").c_str());

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
        std::vector<std::string> defines; // for completeness / diagnostics

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
                    pushBoss(seq);
                hasSection = false;
                seq = BossAccum{};
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
                    if (sec.front() == '[')
                        sec.erase(sec.begin());
                    if (!sec.empty() && sec.back() == ']')
                        sec.pop_back();
                    if (sec.empty())
                    {
                        LOGE("Error: empty section header on line %d.\n", lineNo);
                        return 1;
                    }
                    section = sec;
                    seq.name = section;
                    seq.sheet = sheet;
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
                    // general config
                    if (e[0] == "private" && e.size() >= 3)
                    {
                        defines.push_back(e[1] + " = " + e[2]);
                    }
                    else if (e[0] == "define" && e.size() >= 3)
                    {
                        std::string key = e[1];
                        for (char &c : key)
                            c = static_cast<char>(std::toupper((unsigned char)c));
                        defines.push_back(key + " = " + e[2]);
                    }
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

                if (item == "type")
                    setScalar("type", e[1]);
                else if (item == "hp")
                    setScalar("hp", e[1]);
                else if (item == "score")
                    setScalar("score", e[1]);
                else if (item == "path")
                    setScalar("path", e[1]);
                else if (item == "attack_sound")
                    setScalar("attack_sound", e[1]);
                else if (item == "is_goal")
                    setScalar("is_goal", e[1]);
                else if (item == "show_details")
                    setScalar("show_details", e[1]);
                else if (item == "aims")
                    setScalar("aims", e[1]);
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
            std::string hitboxFile = e.size() == 3 ? e[2] : "";

            if (!hitboxFile.empty())
            {
                std::string full = baseDir + "/" + hitboxFile;
                if (!readHitboxFile(full, GRID_SIZE, frame, sheet, hitboxFrames))
                    return;
            }

            if (e[1].front() == '@')
            {
                // reference to another sequence already defined on this boss
                std::string ref = e[1].substr(1);
                auto it = seq.fields.find(ref + "_base");
                if (it != seq.fields.end())
                {
                    boss_seq_t s{};
                    s.base = std::atoi(it->second.c_str());
                    auto lit = seq.fields.find(ref + "_len");
                    s.lenght = lit == seq.fields.end() ? 0 : std::atoi(lit->second.c_str());
                    seq.fields[seqName + "_base"] = std::to_string(s.base);
                    seq.fields[seqName + "_len"] = std::to_string(s.lenght);
                }
                return;
            }

            int count = std::atoi(e[1].c_str());
            seq.fields[seqName + "_base"] = std::to_string(frame);
            seq.fields[seqName + "_len"] = std::to_string(count);

            int aims = seq.fields.count("aims") ? std::atoi(seq.fields.at("aims").c_str()) : 1;
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

// Leak-pooled storage kept alive until freeBossData() is called, so the
// returned pointers (g_bossData, g_hitboxes, and every d.name) stay valid.
static std::vector<bossData_t> *g_pooledBosses = nullptr;
static std::vector<std::string> *g_pooledNames = nullptr;
static std::vector<sprite_hitbox_t> *g_pooledHitboxes = nullptr;

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
        s.count = static_cast<int>(fr.boxes.size());
        for (size_t i = 0; i < fr.boxes.size() && i < MAX_HITBOX_PER_FRAME; ++i)
            s.hitboxes[i] = fr.boxes[i];
        hb.push_back(s);
    }

    // Adopt the loader's storage into leak-pooled statics.
    g_pooledBosses = new std::vector<bossData_t>(std::move(loader.bosses));
    g_pooledNames = new std::vector<std::string>(std::move(loader.namePool));
    g_pooledHitboxes = new std::vector<sprite_hitbox_t>(std::move(hb));

    // Re-point each boss's name into the pooled names array (build() pointed
    // into the transient BossAccum before the move).
    for (size_t i = 0; i < g_pooledBosses->size(); ++i)
        (*g_pooledBosses)[i].name = g_pooledNames->at(i).c_str();

    g_bossData = g_pooledBosses->data();
    g_bossCount = static_cast<int>(g_pooledBosses->size());
    g_hitboxes = g_pooledHitboxes->data();
    g_hitboxCount = static_cast<int>(g_pooledHitboxes->size());

    return 0;
}

void freeBossData()
{
    delete g_pooledBosses;
    delete g_pooledNames;
    delete g_pooledHitboxes;
    g_pooledBosses = nullptr;
    g_pooledNames = nullptr;
    g_pooledHitboxes = nullptr;
    g_bossData = nullptr;
    g_hitboxes = nullptr;
    g_bossCount = 0;
    g_hitboxCount = 0;
}
