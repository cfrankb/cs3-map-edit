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
#include "shared/FileWrap.h"
#include "logger.h"

namespace MapArchPrivate
{
    constexpr const char MAAZ_SIG[]{'M', 'A', 'A', 'Z'};
    constexpr uint16_t MAAZ_VERSION = 0;
    enum
    {
        OFFSET_COUNT = 6,
        OFFSET_INDEX = 8,
        MAX_MAPS = 1000,
    };
};

using namespace MapArchPrivate;

// Define header structure
struct Header
{
    uint8_t sig[sizeof(MAAZ_SIG)];
    uint16_t version;
    uint16_t count;
    uint32_t offset;
};

/**
 * @brief get the last error
 *
 * @return const char*
 */
const char *CMapArch::lastError()
{
    return m_lastError.c_str();
}

/**
 * @brief get map count
 *
 * @return size_t
 */
size_t CMapArch::size()
{
    return m_maps.size();
}

/**
 * @brief clear the maparch. remove all maps.
 *
 */
void CMapArch::clear()
{
    m_maps.clear();
}

/**
 * @brief add new map
 *
 * @param map
 * @return size_t mapIndex pos
 */

size_t CMapArch::add(std::unique_ptr<CMap> &map)
{
    m_maps.emplace_back(std::move(map));
    return m_maps.size() - 1;
}

/**
 * @brief remove map at index
 *
 * @param i
 * @return CMap*
 */

std::unique_ptr<CMap> CMapArch::removeAt(int i)
{
    if (i < 0 || i >= static_cast<int>(m_maps.size()))
        return nullptr;
    std::unique_ptr<CMap> map = std::move(m_maps[i]);
    m_maps.erase(m_maps.begin() + i);
    return map;
}

/**
 * @brief insert map at index
 *
 * @param i
 * @param map
 */

void CMapArch::insertAt(int i, std::unique_ptr<CMap> map)
{
    if (i < 0 || i > static_cast<int>(m_maps.size()))
        return;
    m_maps.insert(m_maps.begin() + i, std::move(map));
}

/**
 * @brief Deserialize the data from IFile Interface object
 *
 * @param file
 * @return true
 * @return false
 */

bool CMapArch::read(IFile &file)
{
    auto readfile = [&file](auto ptr, auto size) -> bool
    {
        return file.read(ptr, size) == 1;
    };

    auto seekfile = [&file](uint32_t offset) -> bool
    {
        file.seek(offset);
        return offset;
    };

    auto readmap = [&file](std::unique_ptr<CMap> &map) -> bool
    {
        return map->read(file);
    };

    return readCommon(readfile, seekfile, readmap);
}

bool CMapArch::read(const std::string_view &filename)
{
    FILE *sfile = fopen(filename.data(), "rb");
    if (!sfile)
    {
        m_lastError = "can't read file[0]";
        return false;
    }

    auto readfile = [sfile](auto ptr, auto size) -> bool
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    auto seekfile = [sfile](uint32_t offset) -> bool
    {
        return fseek(sfile, offset, SEEK_SET) == 0;
    };

    auto readmap = [sfile](std::unique_ptr<CMap> &map) -> bool
    {
        return map->read(sfile);
    };

    bool result = readCommon(readfile, seekfile, readmap);
    fclose(sfile);
    return result;
}

template <typename ReadFunc, typename SeekFunc, typename ReadMapFunc>
bool CMapArch::readCommon(ReadFunc readfile, SeekFunc seekfile, ReadMapFunc readmap)
{
    Header hdr;

    // read header
    if (readfile(&hdr, sizeof(Header)) != IFILE_OK)
    {
        m_lastError = "Failed to read header";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    // check signature
    if (memcmp(hdr.sig, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
    {
        m_lastError = "MAAZ signature is incorrect";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    // check version
    if (hdr.version != MAAZ_VERSION)
    {
        m_lastError = "MAAZ Version is incorrect";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    // read index
    if (!seekfile(hdr.offset))
    {
        m_lastError = "Failed to seek to index";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    std::vector<uint32_t> indexPtr(hdr.count);
    if (!readfile(indexPtr.data(), sizeof(uint32_t) * hdr.count))
    {
        m_lastError = "Failed to read index";
        LOGE("%s", m_lastError.c_str());
        return false;
    }

    // read levels
    m_maps.resize(hdr.count);
    int i = 0;
    for (auto &map : m_maps)
    {
        if (!seekfile(indexPtr[i]))
        {
            m_lastError = "Failed to seek to map data";
            LOGE("%s", m_lastError.c_str());
            return false;
        }

        if (!map)
            map = std::make_unique<CMap>();
        if (!readmap(map))
        {
            m_lastError = "Failed to read map data [ma]";
            LOGE("%s", m_lastError.c_str());
            return false;
        }
        ++i;
    }
    return true;
}

/**
 * @brief Write file to disk
 *
 * @param filename
 * @return true
 * @return false
 */
bool CMapArch::write(const std::string_view &filename)
{
    bool result;
    // write levelArch
    FILE *tfile = fopen(filename.data(), "wb");
    if (tfile)
    {
        std::vector<long> index;
        // write temp header
        fwrite(MAAZ_SIG, sizeof(MAAZ_SIG), 1, tfile);
        fwrite("\0\0\0\0", 4, 1, tfile);
        fwrite("\0\0\0\0", 4, 1, tfile);
        for (size_t i = 0; i < m_maps.size(); ++i)
        {
            // write maps
            index.emplace_back(ftell(tfile));
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

/**
 * @brief get file signature
 *
 * @return const char*
 */
const char *CMapArch::signature()
{
    return MAAZ_SIG;
}

/**
 * @brief extract map from file. this allows reading maparch, individual map files and legacy files.
 *
 * @param filename
 * @return true
 * @return false
 */
bool CMapArch::extract(const std::string_view &filename)
{
    FILE *sfile = fopen(filename.data(), "rb");
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (!sfile)
    {
        m_lastError = "can't read header";
        return false;
    }
    char sig[sizeof(MAAZ_SIG)];
    readfile(sig, sizeof(MAAZ_SIG));
    fclose(sfile);

    if (memcmp(sig, MAAZ_SIG, sizeof(MAAZ_SIG)) == 0)
    {
        return read(filename);
    }
    else
    {
        clear();
        std::unique_ptr<CMap> map = std::make_unique<CMap>();
        m_maps.emplace_back(std::move(map));
        return fetchLevel(*m_maps[0], filename.data(), m_lastError);
    }
}

/**
 * @brief get mapIndex from file. create a vector of map offsets within the file
 *
 * @param filename
 * @param index
 * @return true
 * @return false
 */
bool CMapArch::indexFromFile(const char *filename, IndexVector &index)
{
    typedef struct
    {
        uint8_t sig[sizeof(MAAZ_SIG)];
        uint16_t version;
        uint16_t count;
        uint32_t offset;
    } Header;

    CFileWrap file;
    if (!file.open(filename, "rb"))
    {
        LOGE("Failed to open file: %s", filename);
        return false;
    }

    Header hdr;

    // Read header
    if (file.read(&hdr, sizeof(Header)) != IFILE_OK)
    {
        LOGE("Failed to read header from %s", filename);
        return false;
    }
    // check signature
    if (memcmp(hdr.sig, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
    {
        return false;
    }
    // check version
    if (hdr.version != MAAZ_VERSION)
    {
        LOGE("Unsupported MAAZ version %d in %s", hdr.version, filename);
        return false;
    }
    if (hdr.count > MAX_MAPS)
    {
        LOGE("Invalid map count %d in %s", hdr.count, filename);
        return false;
    }
    if (hdr.offset == 0)
    {
        LOGE("Invalid offset table position in %s", filename);
        return false;
    }
    const long filesize = file.getSize();

    // Seek to offset table
    file.seek(hdr.offset);
    /* if (!file.seek(hdr.offset))
    {
        LOGE("Failed to seek to offset table in %s", filename);
        return false;
    }*/
    std::vector<uint32_t> offsets(hdr.count);
    if (file.read(offsets.data(), sizeof(uint32_t) * hdr.count) != IFILE_OK)
    {
        LOGE("Failed to read offsets from %s", filename);
        return false;
    }
    index.clear();
    for (uint32_t off : offsets)
    {
        if (off > (uint32_t)filesize)
        {
            LOGE("Invalid offset %u in %s", off, filename);
            return false;
        }
        index.emplace_back(static_cast<long>(off));
    }
    return true;
}

/**
 * @brief create an index from memory. create a vector of map offsets within the memory blob
 *
 * @param ptr
 * @param index
 * @return true
 * @return false
 */

bool CMapArch::indexFromMemory(uint8_t *ptr, IndexVector &index)
{
    if (!ptr)
    {
        LOGE("Null pointer passed to indexFromMemory");
        return false;
    }

    constexpr size_t headerSize = sizeof(Header);
    if (memcmp(ptr, MAAZ_SIG, sizeof(MAAZ_SIG)) != 0)
    {
        LOGE("Invalid MAAZ signature in memory buffer");
        return false;
    }

    Header hdr;
    memcpy(&hdr, ptr, headerSize); // Safe: headerSize is fixed
    if (hdr.version != MAAZ_VERSION)
    {
        LOGE("Unsupported MAAZ version %d in memory buffer", hdr.version);
        return false;
    }
    if (hdr.count > MAX_MAPS)
    {
        LOGE("Invalid map count %d in memory buffer", hdr.count);
        return false;
    }
    if (hdr.offset < headerSize)
    {
        LOGE("Invalid offset table position %u in memory buffer", hdr.offset);
        return false;
    }

    // Read offset table
    index.clear();
    index.reserve(hdr.count);
    ptr += hdr.offset; // Move to offset table
    for (uint16_t i = 0; i < hdr.count; ++i)
    {
        uint32_t offset;
        memcpy(&offset, ptr, sizeof(uint32_t)); // Safe: fixed size
        ptr += sizeof(uint32_t);
        if (offset < headerSize)
        {
            LOGE("Invalid map offset %u at index %d", offset, i);
            return false;
        }
        index.emplace_back(static_cast<long>(offset));
    }

    return true;
}

/**
 * @brief remove all the maps from the maparch without deleting the memory pointers
 *
 */
void CMapArch::removeAll()
{
    m_maps.clear();
}

/**
 * @brief Read maps from a memory blob
 *
 * @param ptr
 * @return true
 * @return false
 */
bool CMapArch::fromMemory(uint8_t *ptr)
{
    uint8_t *org = ptr;
    auto copyData = [&ptr](auto dest, auto size)
    {
        memcpy(dest, ptr, size);
        ptr += size;
        return true;
    };

    auto seekmem = [&ptr, org](uint32_t offset) -> bool
    {
        ptr = org + offset;
        return true;
    };

    auto readmap = [&ptr](std::unique_ptr<CMap> &map) -> bool
    {
        return map->fromMemory(ptr);
    };

    return readCommon(copyData, seekmem, readmap);
}

void CMapArch::debug()
{
    LOGI("maps: %lu", m_maps.size());
    int i = 0;
    for (const auto &map : m_maps)
    {
        LOGI("map %d: %p", i, map.get());
        ++i;
    }
}
