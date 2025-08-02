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
#include "maparch.h"
#include "map.h"
#include "level.h"
#include "shared/IFile.h"

constexpr const char MAAZ_SIG[]{'M', 'A', 'A', 'Z'};
const uint16_t MAAZ_VERSION = 0;

CMapArch::CMapArch()
{
    m_max = GROW_BY;
    m_size = 0;
    m_maps = new CMap *[m_max];
    memset(m_maps, 0, sizeof(CMap *) * m_max);
}

CMapArch::~CMapArch()
{
    forget();
}

const char *CMapArch::lastError()
{
    return m_lastError.c_str();
}

int CMapArch::size()
{
    return m_size;
}

void CMapArch::forget()
{
    if (m_maps)
    {
        for (int i = 0; i < m_size; ++i)
        {
            if (m_maps[i])
                delete m_maps[i];
        }
        delete[] m_maps;
        m_maps = nullptr;
    }
    m_size = 0;
    m_max = 0;
}

int CMapArch::add(CMap *map)
{
    allocSpace();
    m_maps[m_size] = map;
    return m_size++;
}

CMap *CMapArch::removeAt(int i)
{
    CMap *map = m_maps[i];
    for (int j = i; j < m_size - 1; ++j)
    {
        m_maps[j] = m_maps[j + 1];
    }
    --m_size;
    return map;
}

void CMapArch::insertAt(int i, CMap *map)
{
    allocSpace();
    for (int j = m_size; j > i; --j)
    {
        m_maps[j] = m_maps[j - 1];
    }
    ++m_size;
    m_maps[i] = map;
}

void CMapArch::allocSpace()
{
    if (m_size >= m_max)
    {
        m_max = m_size + GROW_BY;
        CMap **t = new CMap *[m_max];
        for (int i = 0; i < m_max; ++i)
        {
            t[i] = i < m_size ? m_maps[i] : nullptr;
        }
        delete[] m_maps;
        m_maps = t;
    }
}

CMap *CMapArch::at(int i)
{
    return m_maps[i];
}

bool CMapArch::read(IFile &file)
{
    auto readfile = [&file](auto ptr, auto size)
    {
        return file.read(ptr, size);
    };

    typedef struct
    {
        uint8_t sig[4];
        uint16_t version;
        uint16_t count;
        uint32_t offset;
    } Header;

    Header hdr;
    // read header
    readfile(&hdr, 12);
    // check signature
    if (memcmp(hdr.sig, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
    {
        m_lastError = "MAAZ signature is incorrect";
        return false;
    }
    // check version
    if (hdr.version != MAAZ_VERSION)
    {
        m_lastError = "MAAZ Version is incorrect";
        return false;
    }
    // read index
    file.seek(hdr.offset);
    uint32_t *indexPtr = new uint32_t[hdr.count];
    readfile(indexPtr, 4 * hdr.count);
    forget();
    m_maps = new CMap *[hdr.count];
    m_size = hdr.count;
    m_max = m_size;
    // read levels
    for (int i = 0; i < hdr.count; ++i)
    {
        file.seek(indexPtr[i]);
        m_maps[i] = new CMap();
        if (!m_maps[i]->read(file))
        {
            return false;
        }
    }
    delete[] indexPtr;
    return true;
}

bool CMapArch::read(const char *filename)
{
    // read levelArch
    typedef struct
    {
        uint8_t sig[4];
        uint16_t version;
        uint16_t count;
        uint32_t offset;
    } Header;

    FILE *sfile = fopen(filename, "rb");
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (sfile)
    {
        Header hdr;
        // read header
        readfile(&hdr, sizeof(Header));
        // check signature
        if (memcmp(hdr.sig, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
        {
            m_lastError = "MAAZ signature is incorrect";
            return false;
        }
        // check version
        if (hdr.version != MAAZ_VERSION)
        {
            m_lastError = "MAAZ Version is incorrect";
            return false;
        }
        // read index
        fseek(sfile, hdr.offset, SEEK_SET);
        uint32_t *indexPtr = new uint32_t[hdr.count];
        readfile(indexPtr, sizeof(uint32_t) * hdr.count);
        forget();
        m_maps = new CMap *[hdr.count];
        m_size = hdr.count;
        m_max = m_size;
        // read levels
        for (int i = 0; i < hdr.count; ++i)
        {
            fseek(sfile, indexPtr[i], SEEK_SET);
            m_maps[i] = new CMap();
            m_maps[i]->read(sfile);
        }
        delete[] indexPtr;
        fclose(sfile);
    }
    else
    {
        m_lastError = "can't read file[0]";
    }
    return sfile != nullptr;
}

bool CMapArch::write(const char *filename)
{
    bool result;
    // write levelArch
    FILE *tfile = fopen(filename, "wb");
    if (tfile)
    {
        std::vector<long> index;
        // write temp header
        fwrite(MAAZ_SIG, sizeof(MAAZ_SIG), 1, tfile);
        fwrite("\0\0\0\0", 4, 1, tfile);
        fwrite("\0\0\0\0", 4, 1, tfile);
        for (int i = 0; i < m_size; ++i)
        {
            // write maps
            index.push_back(ftell(tfile));
            m_maps[i]->write(tfile);
        }
        // write index
        long indexPtr = ftell(tfile);
        size_t size = index.size();
        for (unsigned int i = 0; i < index.size(); ++i)
        {
            long ptr = index[i];
            fwrite(&ptr, 4, 1, tfile);
        }
        // write version
        fseek(tfile, 4, SEEK_SET);
        fwrite(&MAAZ_VERSION, 2, 1, tfile);
        // write size
        fseek(tfile, 6, SEEK_SET);
        fwrite(&size, 2, 1, tfile);
        // write indexPtr
        fwrite(&indexPtr, 4, 1, tfile);
        fclose(tfile);
    }
    else
    {
        m_lastError = "can't write file";
    }
    result = tfile != nullptr;
    return result;
}

const char *CMapArch::signature()
{
    return MAAZ_SIG;
}

bool CMapArch::extract(const char *filename)
{
    FILE *sfile = fopen(filename, "rb");
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (!sfile)
    {
        m_lastError = "can't read header";
        return false;
    }
    char sig[4];
    readfile(sig, 4);
    fclose(sfile);

    if (memcmp(sig, MAAZ_SIG, sizeof(MAAZ_SIG)) == 0)
    {
        return read(filename);
    }
    else
    {
        m_size = 1;
        return fetchLevel(*m_maps[0], filename, m_lastError);
    }
}

bool CMapArch::indexFromFile(const char *filename, IndexVector &index)
{
    typedef struct
    {
        uint8_t sig[4];
        uint16_t version;
        uint16_t count;
        uint32_t offset;
    } Header;

    FILE *sfile = fopen(filename, "rb");
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (sfile)
    {
        Header hdr;
        readfile(&hdr, sizeof(Header));
        // check signature
        if (memcmp(hdr.sig, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
        {
            return false;
        }
        fseek(sfile, hdr.offset, SEEK_SET);
        uint32_t *indexPtr = new uint32_t[hdr.count];
        readfile(indexPtr, sizeof(uint32_t) * hdr.count);
        index.clear();
        for (int i = 0; i < hdr.count; ++i)
        {
            index.push_back(indexPtr[i]);
        }
        delete[] indexPtr;
        fclose(sfile);
    }
    return sfile != nullptr;
}

bool CMapArch::indexFromMemory(uint8_t *ptr, IndexVector &index)
{
    // check signature
    if (memcmp(ptr, MAAZ_SIG, 4) != 0)
    {
        return false;
    }
    index.clear();
    uint16_t count = 0;
    memcpy(&count, ptr + 6, sizeof(count));
    uint32_t indexBase = 0;
    memcpy(&indexBase, ptr + 8, sizeof(indexBase));
    for (uint16_t i = 0; i < count; ++i)
    {
        long idx = 0;
        memcpy(&idx, ptr + indexBase + i * 4, 4);
        index.push_back(idx);
    }
    return true;
}

void CMapArch::removeAll()
{
    m_size = 0;
}
