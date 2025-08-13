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

#include <inttypes.h>

struct AnimzInfo
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
    uint8_t at(uint8_t tileID);
    uint8_t offset();
    bool isSpecialCase(uint8_t tileID);
    AnimzInfo specialInfo(const int tileID);

private:
    using animzSeq_t = struct
    {
        uint8_t srcTile;
        uint8_t startSeq;
        uint8_t count;
        uint8_t specialID;
    };

    enum : uint32_t
    {
        NO_ANIMZ = 255,
        MAX_TILES = 256,
        NA = 0,
    };

    static animzSeq_t m_animzSeq[];
    uint8_t m_tileReplacement[MAX_TILES];
    int32_t *m_seqIndex = nullptr;
    uint8_t m_offset = 0;
};
