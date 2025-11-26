/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

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
#pragma once

#include <functional>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <string>
#include "dirs.h"
#include "logger.h"
#include "shared/helper.h"
#include "shared/IFile.h"
#include "zlib.h"

class CMap;

class CLayer
{

public:
    enum LayerType : uint8_t
    {
        LAYER_MAIN,
        LAYER_FLOOR,
        LAYER_WALLS,
        LAYER_DECOR,
    };

    CLayer(const uint16_t len, const uint16_t hei, const LayerType layerType = LAYER_MAIN, uint16_t baseID = 0, const char *name = "untitled")
    {
        m_len = len;
        m_hei = hei;
        m_tiles.resize(len * hei);
        m_layerType = layerType;
        m_name = name;
        m_baseID = baseID;
    };
    ~CLayer() = default;

    inline bool isValid(const int x, const int y) const
    {
        return x >= 0 && x < m_len && y >= 0 && y < m_hei;
    }

    bool resize(uint16_t in_len, uint16_t in_hei, uint8_t t, bool fast);
    bool shift(Direction aim);
    inline void clear() { m_tiles.clear(); }
    void fill(uint8_t ch = 0);
    size_t size() const { return m_tiles.size(); }
    void replaceTile(const uint8_t, const uint8_t);
    inline int width() const { return m_len; };
    inline int height() const { return m_hei; };
    const char *lastError() { return m_lastError.c_str(); }

    inline uint8_t &get(const int x, const int y)
    {
        if (!isValid(x, y))
        {
            LOGE("invalid coordonates [get] (%d, %d) -- upper bound(%d,%d)", x, y, m_len, m_hei);
            throw std::out_of_range("Invalid map access");
        }
        return m_tiles[x + y * m_len];
    }

    inline uint8_t at(const int x, const int y) const
    {
        if (!isValid(x, y))
        {
            LOGE("invalid coordonates [at] (%d, %d) -- upper bound(%d,%d)", x, y, m_len, m_hei);
            throw std::out_of_range("Invalid map access");
        }
        return m_tiles[x + y * m_len];
    }

    inline void set(const int x, const int y, const uint8_t t)
    {
        get(x, y) = t;
    }

    void setName(const std::string_view &name)
    {
        m_name = name;
    }

    const char *getName()
    {
        return m_name.c_str();
    }

    LayerType layerType() { return m_layerType; }
    inline uint16_t baseID() { return m_baseID; }
    void setBaseID(int baseID) { m_baseID = baseID; };

    std::vector<uint8_t> &tiles() { return m_tiles; };
    const std::vector<uint8_t> &tilesConst() const { return m_tiles; };
    void tilesFrom(const std::vector<uint8_t> &tiles) { m_tiles = tiles; };

protected:
    enum : uint16_t
    {
        MAX_SIZE = 256,
    };

    template <typename WriteFunc>
    inline bool writeCommon(WriteFunc writefile) const
    {
        if (!writefile(&m_len, sizeof(uint8_t)))
            return false;
        if (!writefile(&m_hei, sizeof(uint8_t)))
            return false;
        // if (!writefile(m_tiles.data(), m_len * m_hei))
        //    return false;

        if (!writefile(&m_layerType, sizeof(m_layerType)))
        {
            LOGE("fail to write layer type");
            return false;
        }
        std::vector<u_int8_t> compr;
        const int err = compressData(m_tiles, compr);
        if (err != Z_OK)
        {
            LOGE("Zlib compression error %d: %s", err, zError(err));
            return false;
        }

        size_t size = compr.size();
        if (!writefile(&size, sizeof(uint32_t)))
            return false;
        if (!writefile(compr.data(), size))
            return false;

        // write layer name
        size_t nameSize = m_name.size();
        if (!writefile(&nameSize, sizeof(uint16_t)))
        {
            return false;
        }
        if (nameSize && !writefile(m_name.c_str(), nameSize))
        {
            return false;
        }

        // save baseID;
        if (!writefile(&m_baseID, sizeof(m_baseID)))
        {
            return false;
        }

        return true;
    }
    template <typename ReadFunc>
    inline bool readCommon(ReadFunc &&readfile, const int version)
    {
        // Read map dimensions - preserving original read sizes
        uint16_t len = 0;
        uint16_t hei = 0;
        if (!readfile(&len, sizeof(uint8_t)) || !readfile(&hei, sizeof(uint8_t)))
        {
            m_lastError = "failed to read dimensions";
            LOGE("%s", m_lastError.c_str());
            return false;
        }

        len = len ? len : static_cast<uint16_t>(MAX_SIZE);
        hei = hei ? hei : static_cast<uint16_t>(MAX_SIZE);
        resize(len, hei, 0, true);

        // Read map data
        enum
        {
            VERSION0,
            VERSION1,
            VERSION2,
        };

        if (version == VERSION0)
        {
            m_layerType = LAYER_MAIN;
            if (!readfile(m_tiles.data(), len * hei))
            {
                m_lastError = "failed to read layer data";
                LOGE("%s", m_lastError.c_str());
                return false;
            }
            m_name = "main";
        }
        else if (version == VERSION1 || version == VERSION2)
        {
            if (!readfile(&m_layerType, sizeof(m_layerType)))
            {
                m_lastError = "fail to read layer type";
                LOGE("%s", m_lastError.c_str());
                return false;
            }

            uint32_t compressedSize;
            if (readfile(&compressedSize, sizeof(compressedSize)) != IFILE_OK)
            {
                m_lastError = "Failed to read compressed size";
                return false;
            }

            // read compressed data from disk
            std::vector<uint8_t> cData(compressedSize);
            if (readfile(cData.data(), compressedSize) != IFILE_OK)
            {
                m_lastError = "Failed to read compressed data";
                return false;
            }

            uLong destLen = len * hei;
            int err = uncompress((uint8_t *)m_tiles.data(), &destLen, cData.data(), compressedSize);
            if (err != Z_OK || destLen != len * hei)
            {
                m_lastError = "Zlib decompression error " + std::to_string(err) + ": " + zError(err);
                return false;
            }

            // read layer name
            size_t nameSize = 0;
            m_name = "";
            if (!readfile(&nameSize, sizeof(uint16_t)))
            {
                return false;
            }
            std::vector<char> nameBuffer(nameSize);
            if (nameSize && !readfile(nameBuffer.data(), nameSize))
            {
                return false;
            }
            m_name.assign(nameBuffer.data(), nameBuffer.size());

            if (version == VERSION2)
            {
                // load baseID;
                if (!readfile(&m_baseID, sizeof(m_baseID)))
                {
                    return false;
                }
            }
        }
        else
        {
            m_lastError = "invalid map version";
            LOGE("invalid map version:%u", version);
            return false;
        }
        return true;
    }

private:
    std::string m_name;
    std::string m_lastError;
    std::vector<uint8_t> m_tiles;
    uint16_t m_len;
    uint16_t m_hei;
    uint16_t m_baseID;
    LayerType m_layerType;

    friend class CMap;
};
