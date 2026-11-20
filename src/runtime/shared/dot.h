/*
    LGCK Builder Runtime
    Copyright (C) 2005, 2011  Francois Blanchette

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

// The Dot POD CFrame::draw/save consume. CDotArray was deprecated and
// deleted 2026-11-18 (BUG-DOTARRAY-MOVE): it had zero production callers
// here; only the external LGCK editor rasterizes dots.

struct Dot
{
    Dot(const int _x = 0, const int _y = 0, const uint32_t _color = 0)
    {
        x = _x;
        y = _y;
        color = _color;
    }

    int x;
    int y;
    uint32_t color;
};
