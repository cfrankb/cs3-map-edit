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

using pixel_t = uint32_t;
inline constexpr pixel_t RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept
{
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
}

enum Color : uint32_t
{
    CLEAR = 0,
    ALPHA = 0xff000000,
    WHITE = RGB(0xff, 0xff, 0xff),          // #ffffff
    YELLOW = RGB(0xff, 0xff, 0x00),         // #ffff00
    PURPLE = RGB(0xff, 0x00, 0xff),         // #ff00ff
    DARKPURPLE = RGB(0x4a, 0x45, 0x98),     // #4a4598
    BLACK = RGB(0x00, 0x00, 0x00),          // #000000
    GREEN = RGB(0x00, 0xff, 0x00),          // #00ff00
    DARKGREEN = RGB(0x00, 0x80, 0x00),      // #008000
    LIME = RGB(0xbf, 0xff, 0x00),           // #bfff00
    BLUE = RGB(0x00, 0x00, 0xff),           // #0000ff
    MIDBLUE = RGB(0x00, 0x00, 0x80),        // #000080
    CYAN = RGB(0x00, 0xff, 0xff),           // #00ffff
    RED = RGB(0xff, 0x00, 0x00),            // #ff0000
    DARKRED = RGB(0x80, 0x00, 0x00),        // #800000
    DARKBLUE = RGB(0x00, 0x00, 0x44),       // #000044
    DARKGRAY = RGB(0x44, 0x44, 0x44),       // #444444
    GRAY = RGB(0x88, 0x88, 0x88),           // #808080
    LIGHTGRAY = RGB(0xa9, 0xa9, 0xa9),      // #a9a9a9
    LIGHTSLATEGRAY = RGB(0x77, 0x88, 0x99), // #778899
    ORANGE = RGB(0xf5, 0x9b, 0x14),         // #f59b14
    DARKORANGE = RGB(0xff, 0x8c, 0x00),     // #ff8c00
    CORAL = RGB(0xff, 0x7f, 0x50),          // #ff7f50
    PINK = RGB(0xff, 0xc0, 0xcb),           // #ffc0cb
    HOTPINK = RGB(0xff, 0x69, 0xb4),        // #ff69b4
    DEEPPINK = RGB(0xff, 0x14, 0x93),       // #ff1493
    OLIVE = RGB(0x80, 0x80, 0x00),          // #808000
    MEDIUMSEAGREEN = RGB(0x3C, 0xB3, 0x71), // #3CB371
    SEAGREEN = RGB(0x2E, 0x8B, 0x57),       // #2E8B57
    BLUEVIOLET = RGB(0x8A, 0x2B, 0xE2),     // #8A2BE2
    DEEPSKYBLUE = RGB(0x00, 0xBF, 0xFF),    // #00BFFF
    LAVENDER = RGB(0xE6, 0xE6, 0xFA),       // #E6E6FA
    DARKSLATEGREY = RGB(0x2F, 0x4F, 0x4F),  // #2F4F4F
    BROWN = RGB(150, 75, 0),                // rgb(150, 75, 0)
    RED_BROWN = RGB(165, 42, 42),           // rgb(165, 42, 42)
    PALE_BROWN = RGB(152, 117, 84),         // rgb(152, 117, 84)
    DARK_BROWN = RGB(101, 67, 33),          // rgb(101, 67, 33)
    DARK_BROWN2 = RGB(86, 43, 0),           // rgb(86, 43, 0)
};
