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
#pragma once

#include "tilesdata.h"
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <array>

extern const uint8_t g_specialCases[];

class IFile;

struct animzInfo_t
{
    uint16_t frames;
    uint16_t base;
    uint16_t offset;
};

class CAnimator
{
public:
    CAnimator();
    ~CAnimator();
    void animate();
    inline uint16_t at(uint8_t tileID) const
    {
        return m_tileMainLayer[tileID];
    }
    inline uint16_t getLayerTile(const uint16_t tileID) const
    {
        const uint16_t &c = m_tileLayer[tileID];
        return c ? c : tileID;
    }

    uint16_t offset() const;
    constexpr bool isSpecialCase(uint8_t tileID) const
    {
        for (const auto &val : m_specialCases)
            if (val == tileID)
                return true;
        return false;
    }
    animzInfo_t getSpecialInfo(const int tileID) const;

    bool read(IFile &sfile);
    bool write(IFile &tfile) const;

    struct animzSeq_t
    {
        uint16_t srcTile;   ///< Source tile ID.
        uint16_t startSeq;  ///< Starting frame ID of animation.
        uint16_t count;     ///< Number of frames in sequence.
        uint16_t specialID; ///< Base ID for special animations (0 if none).
    };

    void reloadTileData();

    enum : uint32_t
    {
        NO_ANIMZ = 0xffff,
        MAX_TILES = 256,
        MAX_LAYER_TILES = 1024,
    };

private:
    static inline constexpr const uint8_t m_specialCases[] = {
        TILES_INSECT1,
        TILES_MUSH_IDLE,
        TILES_ZOMBIE,
        TILES_ETURTLE,
        TILES_WHTEWORM,
        TILES_SKELETON,
        TILES_BABYDRAGON,
        TILES_EGG_WHOLE,
    };

    /// Maps tile IDs to current animation frame.
    uint16_t m_tileMainLayer[MAX_TILES];
    uint16_t m_tileLayer[MAX_LAYER_TILES];
    /// Global animation tick counter.
    std::vector<int32_t> m_seqIndex;
    uint16_t m_offset = 0;
    std::unordered_map<uint16_t, animzInfo_t> m_seqLookUp;
};
