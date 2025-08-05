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
#include "map.h"
#include "shared/IFile.h"
#include "states.h"

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

CMap::~CMap()
{
    forget();
    delete m_states;
};

uint8_t &CMap::at(int x, int y)
{
    if (x >= m_len)
    {
        printf("x [%d] greater than m_len\n", x);
    }
    if (y >= m_hei)
    {
        printf("y [%d] greater than m_hei\n", y);
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

void CMap::forget()
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
    auto readfile = [&file](auto ptr, auto size)
    {
        return file.read(ptr, size);
    };

    char sig[4];

    readfile(sig, sizeof(SIG));
    if (memcmp(sig, SIG, sizeof(SIG)) != 0)
    {
        m_lastError = "signature mismatch";
        printf("%s\n", m_lastError.c_str());
        return false;
    }
    uint16_t ver = 0;
    readfile(&ver, sizeof(VERSION));
    if (ver > VERSION)
    {
        m_lastError = "bad version";
        printf("%s\n", m_lastError.c_str());
        return false;
    }
    uint16_t len = 0;
    uint16_t hei = 0;
    readfile(&len, sizeof(uint8_t));
    readfile(&hei, sizeof(uint8_t));
    len = len ? len : MAX_SIZE;
    hei = hei ? hei : MAX_SIZE;
    resize(len, hei, true);
    readfile(m_map, len * hei);
    m_attrs.clear();
    uint16_t attrCount = 0;
    readfile(&attrCount, sizeof(attrCount));
    for (int i = 0; i < attrCount; ++i)
    {
        uint8_t x;
        uint8_t y;
        uint8_t a;
        readfile(&x, sizeof(x));
        readfile(&y, sizeof(y));
        readfile(&a, sizeof(a));
        setAttr(x, y, a);
    }

    // Check for XTR Header
    extrahdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    m_title = "";
    m_states->clear();
    size_t ptr = file.tell();
    if (readfile(&hdr, sizeof(hdr)))
    {
        if ((memcmp(&hdr, XTR_SIG, sizeof(hdr.sig)) == 0))
        {
            // printf("reading4: %s %d\n", hdr.sig, hdr.ver);
            // read title
            uint16_t size = 0;
            if (readfile(&size, 1))
            {
                char tmp[MAX_TITLE + 1];
                tmp[size] = 0;
                if (readfile(tmp, size) != 0)
                {
                    m_title = tmp;
                }
            }
            // read states
            if (hdr.ver >= XTR_VER1)
            {
                m_states->read(file);
            }
        }
        else
        {
            // revert back to previous position
            file.seek(ptr);
        }
    }

    return true;
}

bool CMap::read(FILE *sfile)
{
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    char sig[4];
    readfile(sig, sizeof(SIG));
    if (memcmp(sig, SIG, sizeof(SIG)) != 0)
    {
        m_lastError = "signature mismatch";
        printf("%s\n", m_lastError.c_str());
        return false;
    }
    uint16_t ver = 0;
    readfile(&ver, sizeof(VERSION));
    if (ver > VERSION)
    {
        m_lastError = "bad version";
        printf("%s\n", m_lastError.c_str());
        return false;
    }
    uint16_t len = 0;
    uint16_t hei = 0;
    readfile(&len, sizeof(uint8_t));
    readfile(&hei, sizeof(uint8_t));
    len = len ? len : MAX_SIZE;
    hei = hei ? hei : MAX_SIZE;
    resize(len, hei, true);
    readfile(m_map, len * hei);
    m_attrs.clear();
    uint16_t attrCount = 0;
    readfile(&attrCount, sizeof(attrCount));
    for (int i = 0; i < attrCount; ++i)
    {
        uint8_t x;
        uint8_t y;
        uint8_t a;
        readfile(&x, sizeof(x));
        readfile(&y, sizeof(y));
        readfile(&a, sizeof(a));
        setAttr(x, y, a);
    }

    // Check for XTR Header
    extrahdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    m_title = "";
    m_states->clear();
    size_t ptr = ftell(sfile);
    if (readfile(&hdr, sizeof(hdr)))
    {
        if ((memcmp(&hdr, XTR_SIG, sizeof(hdr.sig)) == 0))
        {
            // printf("reading4: %s %d\n", hdr.sig, hdr.ver);
            // read title
            uint16_t size = 0;
            if (readfile(&size, 1))
            {
                char tmp[MAX_TITLE + 1];
                tmp[size] = 0;
                if (readfile(tmp, size) != 0)
                {
                    m_title = tmp;
                }
            }
            // read states
            if (hdr.ver >= XTR_VER1)
            {
                m_states->read(sfile);
            }
        }
        else
        {
            // revert back to previous position
            fseek(sfile, ptr, SEEK_SET);
        }
    }
    return true;
}

bool CMap::fromMemory(uint8_t *mem)
{
    if (memcmp(mem, SIG, sizeof(SIG)) != 0)
    {
        m_lastError = "signature mismatch";
        printf("%s\n", m_lastError.c_str());
        return false;
    }

    uint16_t ver = 0;
    memcpy(mem + 4, &ver, 2);
    if (ver > VERSION)
    {
        m_lastError = "bad version";
        printf("%s\n", m_lastError.c_str());
        return false;
    }

    uint16_t len = mem[6];
    uint16_t hei = mem[7];
    len = len ? len : MAX_SIZE;
    hei = hei ? hei : MAX_SIZE;
    resize(len, hei, true);
    const uint32_t mapSize = len * hei;
    memcpy(m_map, mem + 8, mapSize);
    m_attrs.clear();
    uint8_t *ptr = mem + 8 + mapSize;
    uint16_t attrCount = 0;
    memcpy(&attrCount, ptr, 2);
    ptr += 2;
    for (int i = 0; i < attrCount; ++i)
    {
        uint8_t x = *ptr++;
        uint8_t y = *ptr++;
        uint8_t a = *ptr++;
        setAttr(x, y, a);
    }
    m_title = "";
    // read title
    extrahdr_t hdr;
    memcpy(&hdr, ptr, sizeof(hdr));
    if ((memcmp(&hdr, XTR_SIG, sizeof(hdr.sig)) == 0) && (hdr.ver == XTR_VER))
    {
        ptr += sizeof(hdr);
        uint8_t size = *ptr;
        ++ptr;
        char tmp[MAX_TITLE + 1];
        memcpy(tmp, ptr, size);
        tmp[size] = 0;
        m_title = tmp;
    }
    return true;
}

bool CMap::write(FILE *tfile)
{
    if (tfile)
    {
        fwrite(SIG, sizeof(SIG), 1, tfile);
        fwrite(&VERSION, sizeof(VERSION), 1, tfile);
        fwrite(&m_len, sizeof(uint8_t), 1, tfile);
        fwrite(&m_hei, sizeof(uint8_t), 1, tfile);
        fwrite(m_map, m_len * m_hei, 1, tfile);
        size_t attrCount = m_attrs.size();
        fwrite(&attrCount, sizeof(uint16_t), 1, tfile);
        for (auto &it : m_attrs)
        {
            uint16_t key = it.first;
            uint8_t x = key & 0xff;
            uint8_t y = key >> 8;
            uint8_t a = it.second;
            fwrite(&x, sizeof(x), 1, tfile);
            fwrite(&y, sizeof(y), 1, tfile);
            fwrite(&a, sizeof(a), 1, tfile);
        }

        ////        if (!m_title.empty() && m_title.size() < MAX_TITLE)
        //    {
        // write title
        extrahdr_t hdr;
        memcpy(&hdr.sig, XTR_SIG, sizeof(hdr.sig));
        hdr.ver = XTR_VER1;
        fwrite(&hdr, sizeof(hdr), 1, tfile);
        int size = m_title.size();
        fwrite(&size, 1, 1, tfile);
        fwrite(m_title.c_str(), m_title.size(), 1, tfile);

        // write states
        m_states->write(tfile);
        //  }
    }
    return tfile != nullptr;
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
            forget();
            m_map = new uint8_t[len * hei];
            if (m_map == nullptr)
            {
                m_lastError = "resize fail";
                printf("%s\n", m_lastError.c_str());
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

void CMap::clear(uint8_t ch)
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

uint8_t CMap::getAttr(const uint8_t x, const uint8_t y)
{
    const uint16_t key = toKey(x, y);
    if (m_attrs.size() > 0 && m_attrs.count(key) != 0)
    {
        return m_attrs[key];
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
    forget();
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

void CMap::debug()
{
    printf("len: %d hei:%d\n", m_len, m_hei);
    printf("attrCount:%ld\n", m_attrs.size());
    for (auto &it : m_attrs)
    {
        uint16_t key = it.first;
        uint8_t x = key & 0xff;
        uint8_t y = key >> 8;
        uint8_t a = it.second;
        printf("key:%.4x x:%.2x y:%.2x a:%.2x\n", key, x, y, a);
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