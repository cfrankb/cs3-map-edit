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

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <array>

struct animzInfo_t
{
    uint8_t frames;
    uint8_t base;
    uint8_t offset;
};

class CAnimator
{
public:
    CAnimator();
    ~CAnimator();
    void animate();
    uint16_t at(uint8_t tileID) const;
    uint16_t offset() const;
    bool isSpecialCase(uint8_t tileID) const;
    animzInfo_t getSpecialInfo(const int tileID) const;

    struct animzSeq_t
    {
        uint16_t srcTile;  ///< Source tile ID.
        uint8_t startSeq;  ///< Starting frame ID of animation.
        uint8_t count;     ///< Number of frames in sequence.
        uint8_t specialID; ///< Base ID for special animations (0 if none).
    };

private:
    enum : uint32_t
    {
        NO_ANIMZ = 255,
        MAX_TILES = 256,
    };
    /// Maps tile IDs to current animation frame.
    std::array<uint8_t, MAX_TILES> m_tileReplacement;
    /// Global animation tick counter.
    std::vector<int32_t> m_seqIndex;
    uint16_t m_offset = 0;
    std::unordered_map<uint16_t, animzInfo_t> m_seqLookUp;
};
