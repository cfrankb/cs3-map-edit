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
#include <list>
#include <string>

enum StateValue : uint16_t
{
    TIMEOUT = 0x01,
    POS_ORIGIN = 0x02,
    POS_EXIT = 0x03,
    MAP_GOAL = 0x04,
    PAR_TIME = 0x05,
    YEAR = 0x06,
    PRIVATE = 0x7f,
    USERDEF1 = 0x80,
    USERDEF2,
    USERDEF3,
    USERDEF4,
    AUTHOR = 0xc0,
    MSG0 = 0xf0,
    MSG1,
    MSG2,
    MSG3,
    MSG4,
    MSG5,
    MSG6,
    MSG7,
    MSG8,
    MSG9,
    MSGA,
    MSGB,
    MSGC,
    MSGD,
    MSGE,
    MSGF
};

enum StateType
{
    TYPE_X,
    TYPE_U,
    TYPE_S,
};

struct KeyOption
{
    std::string display;
    uint16_t value;
};

const std::list<KeyOption> &getKeyOptions();
