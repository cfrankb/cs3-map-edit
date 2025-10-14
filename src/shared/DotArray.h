/*
    LGCK Builder runtime
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
#include <vector>

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

class CDotArray
{
public:
    CDotArray();
    ~CDotArray();

    void add(uint32_t color, int x, int y);
    void add(const Dot &dot);
    bool isEmpty();
    void flush();
    size_t getSize();
    int lineTab(const uint32_t color, const Dot dot1, const Dot dot2, bool clear = true);
    int circle(const uint32_t color, const Dot dot1, const Dot dot2, bool clear = true);
    const Dot &operator[](int i);
    void setLimit(int maxX, int maxY);
    const std::vector<Dot> &dots();

private:
    std::vector<Dot> m_dots;
    size_t m_max;
    int m_maxX;
    int m_maxY;

    enum
    {
        GROWBY = 1000
    };
};
