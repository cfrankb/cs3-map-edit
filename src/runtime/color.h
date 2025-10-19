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

#define RGBA(R, G, B) (R | (G << 8) | (B << 16) | 0xff000000)

enum Color : uint32_t
{
    CLEAR = 0,
    ALPHA = 0xff000000,
    WHITE = RGBA(0xff, 0xff, 0xff),          // #ffffff
    YELLOW = RGBA(0xff, 0xff, 0x00),         // #ffff00
    PURPLE = RGBA(0xff, 0x00, 0xff),         // #ff00ff
    DARKPURPLE = RGBA(0x4a, 0x45, 0x98),     // #4a4598
    BLACK = RGBA(0x00, 0x00, 0x00),          // #000000
    GREEN = RGBA(0x00, 0xff, 0x00),          // #00ff00
    DARKGREEN = RGBA(0x00, 0x80, 0x00),      // #008000
    LIME = RGBA(0xbf, 0xff, 0x00),           // #bfff00
    BLUE = RGBA(0x00, 0x00, 0xff),           // #0000ff
    MIDBLUE = RGBA(0x00, 0x00, 0x80),        // #000080
    CYAN = RGBA(0x00, 0xff, 0xff),           // #00ffff
    RED = RGBA(0xff, 0x00, 0x00),            // #ff0000
    DARKRED = RGBA(0x80, 0x00, 0x00),        // #800000
    DARKBLUE = RGBA(0x00, 0x00, 0x44),       // #000044
    DARKGRAY = RGBA(0x44, 0x44, 0x44),       // #444444
    GRAY = RGBA(0x88, 0x88, 0x88),           // #808080
    LIGHTGRAY = RGBA(0xa9, 0xa9, 0xa9),      // #a9a9a9
    ORANGE = RGBA(0xf5, 0x9b, 0x14),         // #f59b14
    DARKORANGE = RGBA(0xff, 0x8c, 0x00),     // #ff8c00
    CORAL = RGBA(0xff, 0x7f, 0x50),          // #ff7f50
    PINK = RGBA(0xff, 0xc0, 0xcb),           // #ffc0cb
    HOTPINK = RGBA(0xff, 0x69, 0xb4),        // #ff69b4
    DEEPPINK = RGBA(0xff, 0x14, 0x93),       // #ff1493
    OLIVE = RGBA(0x80, 0x80, 0x00),          // #808000
    MEDIUMSEAGREEN = RGBA(0x3C, 0xB3, 0x71), // #3CB371
    SEAGREEN = RGBA(0x2E, 0x8B, 0x57),       // #2E8B57
    BLUEVIOLET = RGBA(0x8A, 0x2B, 0xE2),     // #8A2BE2
    DEEPSKYBLUE = RGBA(0x00, 0xBF, 0xFF),    // #00BFFF
    LAVENDER = RGBA(0xE6, 0xE6, 0xFA),       // #E6E6FA
    DARKSLATEGREY = RGBA(0x2F, 0x4F, 0x4F),  // #2F4F4F
};
