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
#include <vector>
#include <cstdint>
#include <string>
#include "dirs.h"
#include "logger.h"

class CLayer
{

public:
    CLayer(uint16_t len, uint16_t hei)
    {
        m_len = len;
        m_hei = hei;
        m_tiles.resize(len * hei);
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
    int len() const { return m_len; };
    int hei() const { return m_hei; };
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

    enum LayerType : uint8_t
    {
        LAYER_MAIN,
        LAYER_FLOOR,
        LAYER_WALLS,
        LAYER_DECOR,
    };

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
        if (!writefile(m_tiles.data(), m_len * m_hei))
            return false;
        return true;
    }
    template <typename ReadFunc>
    inline bool readImpl(ReadFunc &&readfile)
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
        if (!readfile(m_tiles.data(), len * hei))
        {
            m_lastError = "failed to read layer data";
            LOGE("%s", m_lastError.c_str());
            return false;
        }
        return true;
    }

private:
    std::string m_lastError;
    std::vector<uint8_t> m_tiles;
    uint16_t m_len;
    uint16_t m_hei;

    friend class CMap;
};