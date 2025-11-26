/*
    cs3-runtime-sdl
    Copyright (C) 2024  Francois Blanchette

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
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include "map.h"
#include "shared/IFile.h"
#include "states.h"
#include "logger.h"
#include <cstdint>

namespace MapPrivate
{
    enum
    {
        XTR_VER0 = 0,
        XTR_VER1 = 1,
    };
    constexpr char SIG[]{'M', 'A', 'P', 'Z'};
    constexpr char XTR_SIG[]{"XTR"};

    constexpr uint16_t VERSION = CMap::VERSION2;
    constexpr uint16_t MAX_SIZE = 256;
    constexpr uint16_t MAX_TITLE = 255;
};

using namespace MapPrivate;

typedef struct
{
    char sig[3];
    char ver;
} extrahdr_t;

CMap::CMap(uint16_t len, uint16_t hei, uint8_t t) : m_len(len),
                                                    m_hei(hei), m_states(std::make_unique<CStates>())
{
    addMainLayer();
    resize(len, hei, t, true);
};

CMap::CMap(const CMap &map) : m_len(map.m_len),
                              m_hei(map.m_hei),
                              m_attrs(map.m_attrs),
                              m_title(map.m_title),
                              m_states(std::make_unique<CStates>(*map.m_states))
{
    m_layers.reserve(map.m_layers.size());
    for (const auto &layer : map.m_layers)
    {
        m_layers.emplace_back(new CLayer(*layer));
    }
    if (m_layers.size() == 0)
        addMainLayer();
}

CMap::~CMap() {
    // clear();
};

void CMap::addMainLayer()
{
    m_layers.emplace_back(std::make_unique<CLayer>(m_len, m_hei, CLayer::LayerType::LAYER_MAIN, 0, "main"));
}

void CMap::clear()
{
    m_states->clear();
    m_layers.clear();
    m_len = 0;
    m_hei = 0;
    m_attrs.clear();
}

bool CMap::read(const char *fname)
{
    bool result = false;
    FILE *sfile = fopen(fname, "rb");
    if (sfile)
    {
        result = read(sfile);
        fclose(sfile);
    }
    return sfile != nullptr && result;
}

bool CMap::write(const char *fname) const
{
    bool result = false;
    FILE *tfile = fopen(fname, "wb");
    if (tfile)
    {
        result = write(tfile);
        fclose(tfile);
    }
    return tfile != nullptr && result;
}

bool CMap::read(IFile &file)
{
    auto readfile = [&file](auto ptr, auto size) -> bool
    {
        return file.read(ptr, size) == 1;
    };

    auto tell = [&file]() -> size_t
    {
        return file.tell();
    };

    auto seek = [&file](size_t pos) -> bool
    {
        file.seek(pos);
        return pos;
    };
    auto states = &m_states;
    auto readStates = [&file, states]() -> bool
    {
        return (*states)->read(file);
    };

    return readCommon(readfile, tell, seek, readStates);
}

bool CMap::read(FILE *sfile)
{
    auto readfile = [sfile](auto ptr, auto size) -> bool
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    auto tell = [sfile]() -> size_t
    {
        return ftell(sfile);
    };

    auto seek = [sfile](size_t pos) -> bool
    {
        return fseek(sfile, pos, SEEK_SET) == 0;
    };

    auto states = &m_states;
    auto readStates = [sfile, states]() -> bool
    {
        return (*states)->read(sfile);
    };

    return readCommon(readfile, tell, seek, readStates);
}

template <typename ReadFunc>
bool CMap::readCommon(ReadFunc &&readfile, std::function<size_t()> tell, std::function<bool(size_t)> seek, std::function<bool()> readStates)
{
    // Read and verify signature
    char sig[sizeof(SIG)];
    if (!readfile(sig, sizeof(SIG)))
    {
        m_lastError = "failed to read signature [m]";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    if (memcmp(sig, SIG, sizeof(SIG)) != 0)
    {
        m_lastError = "signature mismatch";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    // Read and verify version
    uint16_t ver = 0;
    if (!readfile(&ver, sizeof(VERSION)))
    {
        m_lastError = "failed to read version";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    if (ver > VERSION)
    {
        m_lastError = "bad version";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    m_layers.clear();

    // read layers
    if (ver >= VERSION1)
    {
        size_t layerCount = 0;
        if (!readfile(&layerCount, sizeof(uint8_t)))
        {
            m_lastError = "failed to read LayerCount";
            LOGE("%s", m_lastError.c_str());
            return false;
        }
        m_layers.reserve(layerCount);
        for (size_t i = 0; i < layerCount; ++i)
        {
            // LOGI("layer %lu", i);
            std::unique_ptr<CLayer> layer = std::make_unique<CLayer>(m_len, m_hei);
            if (!layer->readCommon(readfile, ver))
            {
                m_lastError = std::string("failed read Layer: ") + layer->lastError();
                LOGE("%s", m_lastError.c_str());
                return false;
            }
            m_layers.emplace_back(std::move(layer));
        }
    }
    else
    {
        // add main layer for compatibility w/ older versions
        std::unique_ptr<CLayer> layer = std::make_unique<CLayer>(m_len, m_hei);
        if (!layer->readCommon(readfile, ver))
        {
            m_lastError = std::string("failed read MainLayer: ") + getMainLayer()->lastError();
            LOGE("%s", m_lastError.c_str());
            return false;
        }
        m_layers.emplace_back(std::move(layer));
    }

    m_len = getMainLayer()->width();
    m_hei = getMainLayer()->height();

    // Read attributes
    m_attrs.clear();
    uint16_t attrCount = 0;
    if (!readfile(&attrCount, sizeof(attrCount)))
    {
        m_lastError = "failed to read attribute count";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    for (int i = 0; i < attrCount; ++i)
    {
        uint8_t x, y, a;
        if (!readfile(&x, sizeof(x)) ||
            !readfile(&y, sizeof(y)) ||
            !readfile(&a, sizeof(a)))
        {
            m_lastError = "failed to read attribute data";
            LOGE("%s", m_lastError.c_str());
            return false;
        }
        setAttr(x, y, a);
    }

    // Check for XTR Header
    extrahdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    m_title = "";
    m_states->clear();

    size_t ptr = tell();
    if (readfile(&hdr, sizeof(hdr)))
    {
        // XDR section is optional
        if (memcmp(&hdr, XTR_SIG, sizeof(hdr.sig)) == 0)
        {
            uint16_t size = 0;
            if (readfile(&size, 1) != IFILE_OK)
            {
                LOGE("failed to read map title size");
                return false;
            }
            if (size != 0)
            {
                char tmp[MAX_TITLE + 1];
                tmp[size] = 0;
                if (readfile(tmp, size) != IFILE_OK)
                {
                    LOGE("failed to read map title");
                    return false;
                }
                m_title = tmp;
            }

            // Read states
            if (hdr.ver >= XTR_VER1)
            {
                // This will need adaptation based on your states read method
                return readStates();
            }
        }
        else
        {
            // Revert back to previous position
            seek(ptr);
        }
    }

    return true;
}

bool CMap::fromMemory(uint8_t *mem)
{
    uint8_t *org = mem;
    auto readfile = [&mem](auto ptr, auto size) -> bool
    {
        memcpy(ptr, mem, size);
        mem += size;
        return true;
    };

    auto tell = [&mem, &org]() -> size_t
    {
        return (uint64_t)mem - (uint64_t)org;
    };

    auto seek = [&mem, &org](size_t pos) -> bool
    {
        mem = org + pos;
        return true;
    };

    auto states = &m_states;
    auto readStates = [&mem, states]() -> bool
    {
        return (*states)->fromMemory(mem);
    };

    return readCommon(readfile, tell, seek, readStates);
}

bool CMap::write(FILE *tfile) const
{
    if (!tfile)
        return false;

    auto writefile = [&tfile](const void *ptr, size_t size) -> bool
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };

    if (!writeCommon(writefile))
        return false;
    return m_states->write(tfile);
}

bool CMap::write(IFile &tfile) const
{
    auto writefile = [&tfile](const void *ptr, size_t size) -> bool
    {
        return tfile.write(ptr, size) == 1;
    };

    if (!writeCommon(writefile))
        return false;
    return m_states->write(tfile);
}

template <typename WriteFunc>
bool CMap::writeCommon(WriteFunc writefile) const
{
    // Write header
    if (!writefile(SIG, sizeof(SIG)))
        return false;
    if (!writefile(&VERSION, sizeof(VERSION)))
        return false;

    // write additional layers
    size_t layerCount = m_layers.size();
    if (!writefile(&layerCount, sizeof(uint8_t)))
    {
        LOGE("failed to write layerCount");
        return false;
    }
    for (const auto &layer : m_layers)
    {
        if (!layer->writeCommon(writefile))
            return false;
    }

    // Write attributes
    size_t attrCount = m_attrs.size();
    if (!writefile(&attrCount, sizeof(uint16_t)))
        return false;

    for (const auto &it : m_attrs)
    {
        uint16_t key = it.first;
        uint8_t x = key & 0xff;
        uint8_t y = key >> 8;
        uint8_t a = it.second;

        if (!writefile(&x, sizeof(x)))
            return false;
        if (!writefile(&y, sizeof(y)))
            return false;
        if (!writefile(&a, sizeof(a)))
            return false;
    }

    // Write title header
    extrahdr_t hdr;
    memcpy(&hdr.sig, XTR_SIG, sizeof(hdr.sig));
    hdr.ver = XTR_VER1;
    if (!writefile(&hdr, sizeof(hdr)))
        return false;

    // Fix: Use consistent data type for size
    uint32_t titleSize = static_cast<uint32_t>(m_title.size());
    if (!writefile(&titleSize, sizeof(uint8_t)))
        return false;
    if (m_title.size() > 0)
        if (!writefile(m_title.c_str(), m_title.size()))
            return false;

    return true;
}

const Pos CMap::findFirst(const uint8_t tileId) const
{
    for (int y = 0; y < m_hei; ++y)
    {
        for (int x = 0; x < m_len; ++x)
        {
            if (at(x, y) == tileId)
            {
                return Pos{static_cast<int16_t>(x), static_cast<int16_t>(y)};
            }
        }
    }
    return Pos{NOT_FOUND, NOT_FOUND};
}

size_t CMap::count(const uint8_t tileId) const
{
    size_t count = 0;
    for (uint16_t y = 0; y < m_hei; ++y)
    {
        for (uint16_t x = 0; x < m_len; ++x)
        {
            if (at(x, y) == tileId)
            {
                ++count;
            }
        }
    }
    return count;
}

void CMap::fill(uint8_t ch)
{
    getMainLayer()->fill(ch);
    m_attrs.clear();
}

uint8_t CMap::getAttr(const uint8_t x, const uint8_t y) const
{
    const uint16_t key = toKey(x, y);
    const auto &it = m_attrs.find(key);
    if (it != m_attrs.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

void CMap::setAttr(const uint8_t x, const uint8_t y, const uint8_t a)
{
    const uint16_t key = toKey(x, y);
    if (a == 0)
    {
        m_attrs.erase(key);
    }
    else
    {
        m_attrs[key] = a;
    }
}

const char *CMap::lastError()
{
    return m_lastError.c_str();
}

size_t CMap::size() const
{
    return getMainLayer()->size();
}

CMap &CMap::operator=(const CMap &map)
{
    clear();
    if (this != &map)
    {
        m_len = map.m_len;
        m_hei = map.m_hei;
        m_attrs = map.m_attrs;
        m_title = map.m_title;
        *m_states = *map.m_states;
        for (const auto &layer : map.m_layers)
        {
            m_layers.emplace_back(new CLayer(*layer.get()));
        }
    }
    return *this;
}

bool CMap::shift(const Direction aim)
{
    if (m_len == 0 || m_hei == 0)
        return false; // No-op for empty map

    for (const auto &layer : m_layers)
        if (!layer->shift(aim))
            return false;

    attrMap_t newAttrs; // New attribute map for shifted positions

    // Update attributes
    switch (aim)
    {
    case Direction::UP:
        for (const auto &[key, attr] : m_attrs)
        {
            Pos pos = toPos(key);
            pos.y = (pos.y == 0) ? m_hei - 1 : pos.y - 1;
            newAttrs[toKey(pos.x, pos.y)] = attr;
        }
        break;

    case Direction::DOWN:
        for (const auto &[key, attr] : m_attrs)
        {
            Pos pos = toPos(key);
            pos.y = (pos.y == m_hei - 1) ? 0 : pos.y + 1;
            newAttrs[toKey(pos.x, pos.y)] = attr;
        }
        break;

    case Direction::LEFT:
        for (const auto &[key, attr] : m_attrs)
        {
            Pos pos = toPos(key);
            pos.x = (pos.x == 0) ? m_len - 1 : pos.x - 1;
            newAttrs[toKey(pos.x, pos.y)] = attr;
        }
        break;

    case Direction::RIGHT:
        for (const auto &[key, attr] : m_attrs)
        {
            Pos pos = toPos(key);
            pos.x = (pos.x == m_len - 1) ? 0 : pos.x + 1;
            newAttrs[toKey(pos.x, pos.y)] = attr;
        }
        break;

    default:
        LOGW("Invalid shift direction: %d", static_cast<int>(aim));
        return false;
    }

    m_attrs = std::move(newAttrs); // Update attributes
    return true;
}

uint16_t CMap::toKey(const uint8_t x, const uint8_t y)
{
    return x + (y << 8);
}

uint16_t CMap::toKey(const Pos &pos)
{
    return toKey(pos.x, pos.y);
}

Pos CMap::toPos(const uint16_t key)
{
    return Pos{.x = static_cast<uint8_t>(key & 0xff),
               .y = static_cast<uint8_t>(key >> 8)};
}

void CMap::debug()
{
    LOGI("len: %d hei:%d", m_len, m_hei);
    LOGI("attrCount:%zu", m_attrs.size());
    for (auto &it : m_attrs)
    {
        uint16_t key = it.first;
        uint8_t x = key & 0xff;
        uint8_t y = key >> 8;
        uint8_t a = it.second;
        LOGI("key:%.4x x:%.2x y:%.2x a:%.2x", key, x, y, a);
    }
}

const char *CMap::title()
{
    return m_title.c_str();
}

void CMap::setTitle(const std::string_view &title)
{
    m_title = title;
}

CStates &CMap::states()
{
    return *m_states;
}

bool CMap::resize(uint16_t in_len, uint16_t in_hei, uint8_t t, bool fast)
{
    in_len = std::min(in_len, MAX_SIZE);
    in_hei = std::min(in_hei, MAX_SIZE);

    m_len = in_len;
    m_hei = in_hei;

    for (const auto &layer : m_layers)
        if (!layer->resize(in_hei, in_hei, t, fast))
            return false;

    attrMap_t newAttrs;
    for (const auto &[key, attr] : m_attrs)
    {
        Pos pos = toPos(key);
        if (pos.x >= in_len || pos.y >= in_hei)
            continue;
        newAttrs[toKey(pos.x, pos.y)] = attr;
    }
    m_attrs = std::move(newAttrs); // Update attributes
    return true;
}

void CMap::replaceTile(const uint8_t src, const uint8_t repl)
{
    getMainLayer()->replaceTile(src, repl);
}

CLayer *CMap::addLayer(const CLayer::LayerType lt, const std::string_view &name, uint16_t baseID)
{
    return m_layers.emplace_back(new CLayer(m_len, m_hei, lt, baseID, name.data())).get();
}

CLayer *CMap::getLayer(const size_t index)
{
    if (index < m_layers.size())
        return m_layers[index].get();
    else
        return nullptr;
}

std::unique_ptr<CLayer> CMap::popLayer()
{
    if (m_layers.empty())
        return nullptr;

    // Move out the last element
    std::unique_ptr<CLayer> layer = std::move(m_layers.back());
    m_layers.pop_back(); // cleaner than resize(size-1)
    return layer;
}

void CMap::pushLayer(std::unique_ptr<CLayer> &layer)
{
    m_layers.emplace_back(std::move(layer));
}
