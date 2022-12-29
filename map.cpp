#include "map.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

static const char SIG[] = "MAPZ";
static const uint16_t VERSION = 0;

CMap::CMap(int len, int hei, uint8_t t)
{
    m_size = 0;
    m_len = len;
    m_hei = hei;
    m_map = (m_len * m_hei) != 0 ? new uint8_t[m_len * m_hei] : nullptr;
    if (m_map)
    {
        m_size = m_len * m_hei;
        memset(m_map, t, m_size * sizeof(m_map[0]));
    }
};

CMap::~CMap()
{
    forget();
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
    if (m_map != nullptr)
    {
        delete[] m_map;
        m_map = nullptr;
        m_size = 0;
    }
    m_len = 0;
    m_hei = 0;
    m_size = 0;
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

bool CMap::read(FILE *sfile)
{
    if (sfile)
    {
        char sig[4];
        fread(sig, strlen(SIG), 1, sfile);
        if (memcmp(sig, SIG, strlen(SIG)) != 0)
        {
            m_lastError = "signature mismatch";
            printf("%s\n", m_lastError.c_str());
            return false;
        }
        uint16_t ver = 0;
        fread(&ver, sizeof(VERSION), 1, sfile);
        if (ver > VERSION)
        {
            m_lastError = "bad version";
            printf("%s\n", m_lastError.c_str());
            return false;
        }
        uint8_t len;
        uint8_t hei;
        fread(&len, sizeof(len), 1, sfile);
        fread(&hei, sizeof(hei), 1, sfile);
        resize(len, hei, true);
        fread(m_map, len * hei, 1, sfile);
        m_attrs.clear();
        uint16_t attrCount = 0;
        fread(&attrCount, sizeof(attrCount), 1, sfile);
        for (int i=0; i < attrCount; ++i) {
            uint8_t x;
            uint8_t y;
            uint8_t a;
            fread(&x, sizeof(x), 1, sfile);
            fread(&y, sizeof(y), 1, sfile);
            fread(&a, sizeof(a), 1, sfile);
            setAttr(x,y,a);
        }
    }
    return sfile != nullptr;
}

bool CMap::write(FILE *tfile)
{
    if (tfile)
    {
        fwrite(SIG, strlen(SIG), 1, tfile);
        fwrite(&VERSION, sizeof(VERSION), 1, tfile);
        fwrite(&m_len, sizeof(uint8_t), 1, tfile);
        fwrite(&m_hei, sizeof(uint8_t), 1, tfile);
        fwrite(m_map, m_len * m_hei, 1, tfile);
        size_t attrCount =  m_attrs.size();
        fwrite(&attrCount, sizeof(uint16_t), 1, tfile);
        for (auto& it: m_attrs) {
            uint16_t key = it.first;
            uint8_t x = key & 0xff;
            uint8_t y = key >> 8;
            uint8_t a = it.second;
            fwrite(&x, sizeof(x), 1, tfile);
            fwrite(&y, sizeof(y), 1, tfile);
            fwrite(&a, sizeof(a), 1, tfile);
        }
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

bool CMap::resize(int len, int hei, bool fast)
{
    if (fast) {
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
            m_size = len * hei;
        }
        m_attrs.clear();
    } else {
        uint8_t * newMap = new uint8_t[len * hei];
        memset(newMap, 0, len * hei * sizeof(newMap[0]));
        for (int y=0; y < hei; ++y) {
            memcpy(newMap + y * len,
                   m_map + y * m_len,
                   std::min(static_cast<uint8_t>(len), m_len));
        }
        delete [] m_map;
        m_map = newMap;
        m_size = len * hei;
    }
    m_len = len;
    m_hei = hei;
    return true;
}

const Pos CMap::findFirst(uint8_t tileId)
{
    for (uint8_t y = 0; y < m_hei; ++y)
    {
        for (uint8_t x = 0; x < m_len; ++x)
        {
            if (at(x, y) == tileId)
            {
                return Pos{x, y};
            }
        }
    }
    return Pos{0xff, 0xff};
}

int CMap::count(uint8_t tileId)
{
    int count = 0;
    for (uint8_t y = 0; y < m_hei; ++y)
    {
        for (uint8_t x = 0; x < m_len; ++x)
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

int CMap::size() {
    return m_size;
}

CMap & CMap::operator=(const CMap map)
{
    forget();
    m_len = map.m_len;
    m_hei = map.m_hei;
    m_map = new uint8_t[m_len * m_hei];
    memcpy(m_map, map.m_map, sizeof(m_map[0]) * map.m_len*map.m_hei);
    m_attrs = map.m_attrs;
    m_size = map.m_len*map.m_hei;
    return *this;
}

void CMap::shift(int aim) {

    uint8_t *tmp = new uint8_t[m_len];
    switch (aim){
    case UP:
        memcpy(tmp, m_map, m_len);
        for (int y=1; y < m_hei; ++y) {
            memcpy(m_map + (y - 1) * m_len, m_map + y * m_len, m_len);
        }
        memcpy(m_map + (m_hei -1)* m_len, tmp, m_len);
        break;

    case DOWN:
        memcpy(tmp, m_map + (m_hei - 1) * m_len, m_len);
        for (int y=m_hei -2; y >= 0; --y) {
            memcpy(m_map + (y + 1) * m_len, m_map + y * m_len, m_len);
        }
        memcpy(m_map, tmp, m_len);
        break;

    case LEFT:
        for (int y=0; y < m_hei; ++y) {
            uint8_t *p = m_map + y * m_len;
            memcpy(tmp, p, m_len);
            memcpy(p, tmp+1, m_len-1);
            p[m_len-1] = tmp[0];
        }
        break;
    case RIGHT:
        for (int y=0; y < m_hei; ++y) {
            uint8_t *p = m_map + y * m_len;
            memcpy(tmp, p, m_len);
            memcpy(p+1, tmp, m_len-1);
            p[0] = tmp[m_len-1];
        }
        break;
    };

    uint8_t tx = 0;
    uint8_t ty = 0;

    switch (aim){
    case UP:
        ty = 0xff;
        break;
    case DOWN:
        ty = 1;
        break;
    case LEFT:
        tx = 0xff;
        break;
    case RIGHT:
        tx = 1;
        break;
    }

    AttrMap tMap;
    for (auto& it: m_attrs) {
        uint16_t key = it.first;
        uint8_t x = key & 0xff;
        uint8_t y = key >> 8;
        uint8_t a = it.second;
        uint16_t newKey = toKey(x + tx, y + ty);
        tMap[newKey] = a;
    }
    m_attrs = tMap;
    delete []tmp;
}

uint16_t CMap::toKey(const uint8_t x, const uint8_t y)
{
    return x + (y << 8);
}
