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

#include "../shared/DotArray.h"
#include <cmath>

///////////////////////////////////////////////////////////////////
// CDotArray

CDotArray::CDotArray() {
    m_max = GROWBY;
    m_size = 0;
    m_dots = new Dot[m_max];
    m_maxX = -1;
    m_maxY = -1;
}

CDotArray::~CDotArray() {
    if (m_dots) {
        delete [] m_dots;
    }
}

void CDotArray::add(const Dot &dot)
{
    add(dot.color, dot.x, dot.y);
}

void CDotArray::add(uint32_t color, int x, int y)
{
    if (x >= 0 && y >= 0 && (x < m_maxX || m_maxX ==-1)
            && (y < m_maxY || m_maxY ==-1)) {

        if (m_size==m_max) {
            m_max += GROWBY;
            Dot *t =  new Dot[m_max];
            for (int i=0; i < m_size; ++i) {
                t[i] = m_dots[i];
            }

            delete [] m_dots;
            m_dots = t;
        }

        Dot & dot = m_dots[m_size];
        dot.color = color;
        dot.x = x;
        dot.y = y;
        ++ m_size;
    }
}

void CDotArray::flush()
{
    m_size = 0;
}

bool CDotArray::isEmpty()
{
    return !m_size;
}

Dot & CDotArray::operator[] (int i)
{
    return m_dots[i];
}

int CDotArray::getSize()
{
    return m_size;
}

int CDotArray::lineTab(const uint32_t color, const Dot dot1, const Dot dot2, bool clear)
{
    if (clear) {
        flush();
    }

    int Ydiff;
    int Xdiff;

    int Yunit, Xunit;
    int error;

    int XX;
    int YY;

    XX = dot1.x;
    YY = dot1.y;

    Ydiff = dot2.y - dot1.y;

    if (Ydiff < 0) {
        Ydiff = -Ydiff;
        Yunit = -1;
    }  else   {
        Yunit = 1;
    }

    Xdiff = dot2.x - dot1.x;

    if (Xdiff < 0) {
        Xdiff = -Xdiff;
        Xunit = -1;
    }  else   {
        Xunit = 1;
    }

    error = 0;
    if (Xdiff > Ydiff)  {
        int len = Xdiff;// + 1;
        for (int i=0; i<=len; i++) {
            add(color, XX,YY);
            XX = XX + Xunit;
            error = error + Ydiff;
            if (error > Xdiff)   {
                error = error - Xdiff;
                YY = YY + Yunit;
            }
        }
    } else {
        int len = Ydiff;// + 1;
        for (int i=0; i<=len; i++) {
            add(color, XX,YY);
            YY = YY + Yunit;
            error = error + Xdiff;
            if (error > 0)     {
                error = error - Ydiff;
                XX = XX + Xunit;
            }
        }
    }

    return getSize();
}

int CDotArray::circle(const uint32_t color, const Dot dot1, const Dot dot2, bool clear)
{
    if (clear){
        flush();
    }

    float radius = std::abs( (float) dot1.y - dot2.y) / 2.0f;
    int x0 = dot1.x;
    int y0 = dot1.y - (dot1.y - dot2.y) / 2;
    float tmp = radius * radius;
    float x = -radius;
    do {
        float y = sqrt(tmp - (x * x));
        add(color, x0 + x, y0 - y); // 4.th quadrant
        add(color, x0 + x, y0 + y); // 3.rd quadrant
        add(color, x0 - x, y0 - y); // 1.st quadrant
        add(color, x0 - x, y0 + y); // 1.nd quadrant
        x = x + 0.1f;
    } while (x <= 0);
    return getSize();
}

void CDotArray::setLimit(int maxX, int maxY)
{
    m_maxX = maxX;
    m_maxY = maxY;
}
