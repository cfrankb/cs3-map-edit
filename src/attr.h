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

#define PASSAGE_ATTR_MIN 0x01  // 0x01
#define PASSAGE_ATTR_MAX 0x7f  // 0x7f
#define PASSAGE_REG_MIN 0x01   // 0x01
#define PASSAGE_REG_MAX 0x1f   // 0x1f
#define SECRET_ATTR_MIN 0x40   // 0x40
#define SECRET_ATTR_MAX 0x4f   // 0x4f
#define ATTR_CRUSHERV_MIN 0xd0 // 0xd0 crusherv
#define ATTR_CRUSHERV_MAX 0xd3 // 0xd3 crusherv
#define ATTR_CRUSHERH_MIN 0xd4 // 0xd4 crusherh
#define ATTR_CRUSHERH_MAX 0xd7 // 0xd7 crusherh
#define ATTR_CRUSHER_MIN 0xd0  // 0xd0
#define ATTR_CRUSHER_MAX 0xd7  // 0xd7
#define ATTR_WAIT_MIN 0xe0     // 0xe0 monster wait
#define ATTR_WAIT_MAX 0xe9     // 0xe9 monster wait
#define ATTR_FREEZE_TRAP 0xea  // 0xea freeze trap
#define ATTR_TRAP 0xeb         // 0xeb trap
#define ATTR_MSG_MIN 0xf0      // 0xf0 .. 0xff message string
#define ATTR_MSG_MAX 0xff      // 0xff .. 0xff message string

#define RANGE(_x, _min, _max) (_x >= _min && _x <= _max)
