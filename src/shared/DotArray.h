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

#ifndef X__DOTARRAY__
#define X__DOTARRAY__

#include <stdint.h>

class Dot {

public:
    Dot(int sx=0, int sy=0, uint32_t sColor =0) {
        x = sx;
        y = sy;
        color = sColor;
    }

    uint32_t color;
    int x;
    int y;
};

class CDotArray
{
public:

    CDotArray();
    ~CDotArray();

    void add(uint32_t color, int x, int y);
    void add(const Dot & dot);
    bool isEmpty();
    void flush();
    int getSize();
    int lineTab(const uint32_t color, const Dot dot1, const Dot dot2, bool clear=true);
    int circle(const uint32_t color, const Dot dot1, const Dot dot2, bool clear=true);
    Dot & operator[] (int i);
    void setLimit(int maxX, int maxY);

protected:

    Dot *m_dots;
    int m_size;
    int m_max;
    int m_maxX;
    int m_maxY;

    enum {
        GROWBY = 1000
    };
};

#endif
