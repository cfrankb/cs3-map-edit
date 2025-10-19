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

#define _S(_data) #_data

#define _W(_data, _size)                                                             \
    if (!writefile(_data, _size))                                                    \
    {                                                                                \
        LOGE("failed to write: `%s` size: %lu on line %d", #_data, _size, __LINE__); \
        return false;                                                                \
    }

#define _R(_data, _size)                                                            \
    if (!readfile(_data, _size))                                                    \
    {                                                                               \
        LOGE("failed to read: `%s` size: %lu on line %d", #_data, _size, __LINE__); \
        return false;                                                               \
    }
