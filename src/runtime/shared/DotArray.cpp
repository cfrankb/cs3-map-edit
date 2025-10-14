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

#include "DotArray.h"
#include <cmath>

CDotArray::CDotArray()
{
    m_max = GROWBY;
    m_dots.reserve(m_max);
    m_maxX = -1;
    m_maxY = -1;
}

CDotArray::~CDotArray()
{
}

void CDotArray::add(const Dot &dot)
{
    add(dot.color, dot.x, dot.y);
}

void CDotArray::add(uint32_t color, int x, int y)
{
    if (x >= 0 && y >= 0 && (x < m_maxX || m_maxX == -1) && (y < m_maxY || m_maxY == -1))
    {
        if (m_dots.size() == m_max)
        {
            m_max += GROWBY;
            m_dots.reserve(m_max);
        }
        m_dots.emplace_back(Dot{x, y, color});
    }
}

void CDotArray::flush()
{
    m_dots.clear();
}

bool CDotArray::isEmpty()
{
    return m_dots.size() != 0;
}

const Dot &CDotArray::operator[](int i)
{
    return m_dots[i];
}

size_t CDotArray::getSize()
{
    return m_dots.size();
}

int CDotArray::lineTab(const uint32_t color, const Dot dot1, const Dot dot2, bool clear)
{
    if (clear)
    {
        flush();
    }

    int dx = abs(dot2.x - dot1.x);
    int dy = abs(dot2.y - dot1.y);
    int err = dx - dy; // Initial error
    int x = dot1.x;
    int y = dot1.y;
    int sx = (dot1.x < dot2.x) ? 1 : -1;
    int sy = (dot1.y < dot2.y) ? 1 : -1;
    for (int i = 0; i <= dx; ++i)
    {
        add(color, x, y);
        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }

    return getSize();
}

int CDotArray::circle(const uint32_t color, const Dot dot1, const Dot dot2, bool clear)
{
    if (clear)
    {
        flush();
    }

    // Assume vertical for now; radius = std::abs(dot1.y - dot2.y) / 2;  // int
    int x0 = dot1.x;
    int y0 = (dot1.y + dot2.y) / 2;
    int r = std::abs(dot1.y - dot2.y) / 2;
    if (r == 0)
    {
        add(color, x0, y0);
        return getSize();
    }
    int x = 0, y = r;
    int d = 3 - 2 * r; // Decision param
    // Plot initial points (top/right octants, symmetry for others)
    auto plot_sym = [&](int dx, int dy)
    {
        add(color, x0 + dx, y0 + dy);
        add(color, x0 - dx, y0 + dy);
        add(color, x0 + dx, y0 - dy);
        add(color, x0 - dx, y0 - dy);
    };
    plot_sym(x, y); // Initial
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d += 4 * (x - y) + 10; // Adjust for both regions
        }
        else
        {
            d += 4 * x + 6;
        }
        plot_sym(x, y);
    }
    return getSize();
}

void CDotArray::setLimit(int maxX, int maxY)
{
    m_maxX = maxX;
    m_maxY = maxY;
}

const std::vector<Dot> &CDotArray::dots()
{
    return m_dots;
}