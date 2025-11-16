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
#include <stdexcept>
#include <algorithm>
#include "layer.h"
#include "logger.h"

namespace LayerPrivate
{

    constexpr uint16_t MAX_TITLE = 255;
    constexpr int16_t NOT_FOUND = -1; // 0xffff
};

using namespace LayerPrivate;

bool CLayer::resize(uint16_t in_len, uint16_t in_hei, uint8_t t, bool fast)
{
    in_len = std::min(in_len, static_cast<uint16_t>(MAX_SIZE));
    in_hei = std::min(in_hei, static_cast<uint16_t>(MAX_SIZE));

    if (fast)
    {
        m_tiles.resize(in_len * in_hei, t);
    }
    else
    {
        std::vector<uint8_t> map(in_len * in_hei);
        for (int y = 0; y < std::min(m_hei, in_hei); ++y)
        {
            for (int x = 0; x < std::min(m_len, in_len); ++x)
            {
                map[x + y * in_len] = m_tiles[x + y * m_len];
            }
        }
        m_tiles = std::move(map);
    }

    m_len = in_len;
    m_hei = in_hei;
    return true;
}

bool CLayer::shift(Direction aim)
{
    if (m_len == 0 || m_hei == 0)
        return false; // No-op for empty map

    std::vector<uint8_t> tmp(m_len); // Temporary buffer for row/column

    switch (aim)
    {
    case Direction::UP:
        // Save top row, rotate tiles upward, place top row at bottom
        std::copy(m_tiles.begin(), m_tiles.begin() + m_len, tmp.begin());
        std::rotate(m_tiles.begin(), m_tiles.begin() + m_len, m_tiles.end());
        break;

    case Direction::DOWN:
        // Save bottom row, rotate tiles downward, place bottom row at top
        std::copy(m_tiles.end() - m_len, m_tiles.end(), tmp.begin());
        std::rotate(m_tiles.rbegin(), m_tiles.rbegin() + m_len, m_tiles.rend());
        break;

    case Direction::LEFT:
        // Shift each row left, wrap first column to last
        for (int y = 0; y < m_hei; ++y)
        {
            auto start = m_tiles.begin() + y * m_len;
            tmp[0] = *start; // Save first element
            std::rotate(start, start + 1, start + m_len);
            *(start + m_len - 1) = tmp[0];
        }
        break;

    case Direction::RIGHT:
        // Shift each row right, wrap last column to first
        for (int y = 0; y < m_hei; ++y)
        {
            auto start = m_tiles.begin() + y * m_len;
            tmp[0] = *(start + m_len - 1); // Save last element
            std::rotate(start, start + m_len - 1, start + m_len);
            *start = tmp[0];
        }
        break;

    default:
        LOGW("Invalid shift direction: %d", static_cast<int>(aim));
        return false;
    }

    return true;
}

void CLayer::fill(uint8_t ch)
{
    if (m_len * m_hei > 0)
        for (int i = 0; i < m_len * m_hei; ++i)
            m_tiles[i] = ch;
}

void CLayer::replaceTile(const uint8_t src, const uint8_t repl)
{
    for (auto &tileID : m_tiles)
    {
        if (tileID == src)
            tileID = repl;
    }
}
