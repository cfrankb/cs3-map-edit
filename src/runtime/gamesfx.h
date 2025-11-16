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

enum Sfx : uint16_t;
enum SfxTimeout : uint16_t;

struct sfx_t
{
    int16_t x;
    int16_t y;
    Sfx sfxID;
    uint16_t timeout;

    bool isWithin(const int x1, const int y1, const int x2, const int y2) const
    {
        return (x >= x1) && (x < x2) && (y >= y1) && (y < y2);
    }
};

enum Sfx : uint16_t
{
    SFX_SPARKLE = 0xf0,
    SFX_EXPLOSION1, // fireball explosion
    SFX_EXPLOSION5, // barrel explosion
    SFX_EXPLOSION6, // icecube melting
    SFX_EXPLOSION7, // thunderbolt destruction (yellow)
    SFX_EXPLOSION0, // placeholder for mob
    SFX_FLAME,      // barrel flame
};

enum SfxTimeout : uint16_t
{
    SFX_SPARKLE_TIMEOUT = 200,
    SFX_EXPLOSION1_TIMEOUT = 10,
    SFX_EXPLOSION6_TIMEOUT = 10,
    SFX_EXPLOSION7_TIMEOUT = 10,
    SFX_EXPLOSION0_TIMEOUT = 10,
    SFX_EXPLOSION5_TIMEOUT = 10,
    SFX_FLAME_TIMEOUT = 20,
};
