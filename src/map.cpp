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
#define LOG_TAG "map"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "map.h"
#include "shared/IFile.h"
#include "states.h"
#include "logger.h"

static const char SIG[]{'M', 'A', 'P', 'Z'};
static const char XTR_SIG[]{"XTR"};
static const char XTR_VER = 0;
static const uint16_t VERSION = 0;
static const uint16_t MAX_SIZE = 256;
static const uint16_t MAX_TITLE = 255;

typedef struct
{
    char sig[3];
    char ver;
} extrahdr_t;

CMap::CMap(uint16_t len, uint16_t hei, uint8_t t)
{
    len = std::min(len, MAX_SIZE);
    hei = std::min(hei, MAX_SIZE);
    m_size = 0;
    m_len = len;
    m_hei = hei;
    m_map = (m_len * m_hei) != 0 ? new uint8_t[m_len * m_hei] : nullptr;
    if (m_map)
    {
        m_size = m_len * m_hei;
        memset(m_map, t, m_size * sizeof(m_map[0]));
    }
    m_states = new CStates;
};

CMap::CMap(const CMap &map)
{
    m_len = map.m_len;
    m_hei = map.m_hei;
    m_size = map.m_len * map.m_hei;
    m_map = new uint8_t[m_size];
    memcpy(m_map, map.m_map, m_size);
    m_attrs = map.m_attrs;
    m_title = map.m_title;
    m_states = new CStates();
    *m_states = *map.m_states;
}

CMap::~CMap()
{
    clear();
    delete m_states;
};

uint8_t &CMap::at(int x, int y)
{
    if (x >= m_len)
    {
        LOGW("x [%d] greater than m_len\n", x);
    }
    if (y >= m_hei)
    {
        LOGW("y [%d] greater than m_hei\n", y);
    }
    return *(m_map + x + y * m_len);
}

uint8_t *CMap::row(int y)
{
    return m_map + y * m_len;
}

void CMap::set(int x, int y, uint8_t t)
{
    at(x, y) = t;
}

void CMap::clear()
{
    m_states->clear();
    if (m_map != nullptr)
    {
        delete[] m_map;
        m_map = nullptr;
        m_size = 0;
    }
    m_len = 0;
    m_hei = 0;
    m_size = 0;
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

bool CMap::write(const char *fname)
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
    auto states = m_states;
    auto readStates = [&file, states]() -> bool
    {
        return states->read(file);
    };

    return readImpl(readfile, tell, seek, readStates);
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

    auto states = m_states;
    auto readStates = [sfile, states]() -> bool
    {
        return states->read(sfile);
    };

    return readImpl(readfile, tell, seek, readStates);
}

template <typename ReadFunc>
bool CMap::readImpl(ReadFunc &&readfile, std::function<size_t()> tell, std::function<bool(size_t)> seek, std::function<bool()> readStates)
{
    // Read and verify signature
    char sig[sizeof(SIG)];
    if (!readfile(sig, sizeof(SIG)))
    {
        m_lastError = "failed to read signature [m]";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    if (memcmp(sig, SIG, sizeof(SIG)) != 0)
    {
        m_lastError = "signature mismatch";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    // Read and verify version
    uint16_t ver = 0;
    if (!readfile(&ver, sizeof(VERSION)))
    {
        m_lastError = "failed to read version";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    if (ver > VERSION)
    {
        m_lastError = "bad version";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    // Read map dimensions - preserving original read sizes
    uint16_t len = 0;
    uint16_t hei = 0;
    if (!readfile(&len, sizeof(uint8_t)) || !readfile(&hei, sizeof(uint8_t)))
    {
        m_lastError = "failed to read dimensions";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    len = len ? len : MAX_SIZE;
    hei = hei ? hei : MAX_SIZE;
    resize(len, hei, true);

    // Read map data
    if (!readfile(m_map, len * hei))
    {
        m_lastError = "failed to read map data [m]";
        LOGE("%s\n", m_lastError.c_str());
        return false;
    }

    // Read attributes
    m_attrs.clear();
    uint16_t attrCount = 0;
    if (!readfile(&attrCount, sizeof(attrCount)))
    {
        m_lastError = "failed to read attribute count";
        LOGE("%s\n", m_lastError.c_str());
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
            LOGE("%s\n", m_lastError.c_str());
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
        if (memcmp(&hdr, XTR_SIG, sizeof(hdr.sig)) == 0)
        {
            // Read title - preserving original read size
            uint16_t size = 0;
            if (readfile(&size, 1) && size > 0)
            { // Original uses 1 byte
                char tmp[MAX_TITLE + 1];
                tmp[size] = 0;
                if (readfile(tmp, size) != 0)
                {
                    m_title = tmp;
                }
            }

            // Read states
            if (hdr.ver >= XTR_VER1)
            {
                // This will need adaptation based on your states read method
                readStates();
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

    auto states = m_states;
    auto readStates = [&mem, states]() -> bool
    {
        return states->fromMemory(mem);
    };

    return readImpl(readfile, tell, seek, readStates);
}

bool CMap::write(FILE *tfile)
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

bool CMap::write(IFile &tfile)
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
bool CMap::writeCommon(WriteFunc writefile)
{
    // Write header
    if (!writefile(SIG, sizeof(SIG)))
        return false;
    if (!writefile(&VERSION, sizeof(VERSION)))
        return false;
    if (!writefile(&m_len, sizeof(uint8_t)))
        return false;
    if (!writefile(&m_hei, sizeof(uint8_t)))
        return false;
    if (!writefile(m_map, m_len * m_hei))
        return false;

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
    if (!writefile(m_title.c_str(), m_title.size()))
        return false;

    return true;
}

int CMap::len() const
{
    return m_len;
}
int CMap::hei() const
{
    return m_hei;
}

bool CMap::resize(uint16_t len, uint16_t hei, bool fast)
{
    len = std::min(len, MAX_SIZE);
    hei = std::min(hei, MAX_SIZE);
    if (fast)
    {
        if (len * hei > m_size)
        {
            clear();
            m_map = new uint8_t[len * hei];
            if (m_map == nullptr)
            {
                m_lastError = "resize fail";
                LOGE("%s\n", m_lastError.c_str());
                return false;
            }
        }
        m_attrs.clear();
    }
    else
    {
        uint8_t *newMap = new uint8_t[len * hei];
        memset(newMap, 0, len * hei);
        for (int y = 0; y < std::min(m_hei, hei); ++y)
        {
            memcpy(newMap + y * len,
                   m_map + y * m_len,
                   std::min(len, m_len));
        }
        delete[] m_map;
        m_map = newMap;
    }
    m_size = len * hei;
    m_len = len;
    m_hei = hei;
    return true;
}

const Pos CMap::findFirst(uint8_t tileId)
{
    for (uint16_t y = 0; y < m_hei; ++y)
    {
        for (uint16_t x = 0; x < m_len; ++x)
        {
            if (at(x, y) == tileId)
            {
                return Pos{x, y};
            }
        }
    }
    return Pos{NOT_FOUND, NOT_FOUND};
}

int CMap::count(uint8_t tileId)
{
    int count = 0;
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
    if (m_map && m_len * m_hei > 0)
    {
        for (int i = 0; i < m_len * m_hei; ++i)
        {
            m_map[i] = ch;
        }
    }
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

int CMap::size()
{
    return m_size;
}

CMap &CMap::operator=(const CMap &map)
{
    clear();
    m_len = map.m_len;
    m_hei = map.m_hei;
    m_size = map.m_len * map.m_hei;
    m_map = new uint8_t[m_size];
    memcpy(m_map, map.m_map, m_size);
    m_attrs = map.m_attrs;
    m_title = map.m_title;
    *m_states = *map.m_states;
    return *this;
}

void CMap::shift(int aim)
{
    uint8_t *tmp = new uint8_t[m_len];
    switch (aim)
    {
    case UP:
        memcpy(tmp, m_map, m_len);
        for (int y = 1; y < m_hei; ++y)
        {
            memcpy(m_map + (y - 1) * m_len, m_map + y * m_len, m_len);
        }
        memcpy(m_map + (m_hei - 1) * m_len, tmp, m_len);
        break;

    case DOWN:
        memcpy(tmp, m_map + (m_hei - 1) * m_len, m_len);
        for (int y = m_hei - 2; y >= 0; --y)
        {
            memcpy(m_map + (y + 1) * m_len, m_map + y * m_len, m_len);
        }
        memcpy(m_map, tmp, m_len);
        break;

    case LEFT:
        for (int y = 0; y < m_hei; ++y)
        {
            uint8_t *p = m_map + y * m_len;
            memcpy(tmp, p, m_len);
            memcpy(p, tmp + 1, m_len - 1);
            p[m_len - 1] = tmp[0];
        }
        break;
    case RIGHT:
        for (int y = 0; y < m_hei; ++y)
        {
            uint8_t *p = m_map + y * m_len;
            memcpy(tmp, p, m_len);
            memcpy(p + 1, tmp, m_len - 1);
            p[0] = tmp[m_len - 1];
        }
        break;
    };

    AttrMap tMap;
    for (auto &it : m_attrs)
    {
        uint16_t key = it.first;
        uint16_t x = key & 0xff;
        uint16_t y = key >> 8;
        uint8_t a = it.second;
        switch (aim)
        {
        case UP:
            y = y ? y - 1 : m_hei - 1;
            break;
        case DOWN:
            y = y < m_hei - 1 ? y + 1 : 0;
            break;
        case LEFT:
            x = x ? x - 1 : m_len - 1;
            break;
        case RIGHT:
            x = x < m_len - 1 ? x + 1 : 0;
        }
        uint16_t newKey = toKey(x, y);
        tMap[newKey] = a;
    }
    m_attrs = tMap;
    delete[] tmp;
}

uint16_t CMap::toKey(const uint8_t x, const uint8_t y)
{
    return x + (y << 8);
}

Pos CMap::toPos(const uint16_t key)
{
    return Pos{.x = static_cast<uint8_t>(key & 0xff),
               .y = static_cast<uint8_t>(key >> 8)};
}

void CMap::debug()
{
    LOGI("len: %d hei:%d\n", m_len, m_hei);
    LOGI("attrCount:%zu\n", m_attrs.size());
    for (auto &it : m_attrs)
    {
        uint16_t key = it.first;
        uint8_t x = key & 0xff;
        uint8_t y = key >> 8;
        uint8_t a = it.second;
        LOGI("key:%.4x x:%.2x y:%.2x a:%.2x\n", key, x, y, a);
    }
}

const char *CMap::title()
{
    return m_title.c_str();
}

void CMap::setTitle(const char *title)
{
    m_title = title;
}

CStates &CMap::states()
{
    return *m_states;
}