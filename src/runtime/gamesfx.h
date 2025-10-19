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
#include <cstdint>

struct sfx_t
{
    int16_t x;
    int16_t y;
    uint8_t sfxID;
    uint16_t timeout;

    bool isWithin(const int x1, const int y1, const int x2, const int y2) const
    {
        return (x >= x1) && (x < x2) && (y >= y1) && (y < y2);
    }
};

#define SFX_SPARKLE 0xf0
#define SFX_SPARKLE_TIMEOUT 200
#define SFX_EXPLOSION1 0xf1
#define SFX_EXPLOSION1_TIMEOUT 10
#define SFX_EXPLOSION6 0xf2
#define SFX_EXPLOSION6_TIMEOUT 10
#define SFX_EXPLOSION7 0xf3
#define SFX_EXPLOSION7_TIMEOUT 10