/*
    cs3-runtime-sdl
    Copyright (C) 2025  Francois Blanchette

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

#include <cinttypes>

enum JoyAim : uint8_t
{
    AIM_UP = 0,
    AIM_DOWN = 1,
    AIM_LEFT = 2,
    AIM_RIGHT = 3,
    TOTAL_AIMS = 4,
    AIM_NONE = 0xff,
};

JoyAim operator^=(JoyAim &aim, int i);
JoyAim reverseDir(const JoyAim aim);