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

#include <cmath>
#include "boss.h"
#include "game.h"
#include "tilesdata.h"
#include "sprtypes.h"
#include "logger.h"

CBoss::CBoss(const int16_t x, const int16_t y, const Rect hitbox, const uint8_t type)
{
    m_x = x;
    m_y = y;
    m_type = type;
    m_hitbox = hitbox;
    m_state = Patrol;
}

bool CBoss::isPlayerHere() const
{
    const CMap &map = CGame::getMap();
    const int x = m_x / BOSS_GRANULAR_FACTOR;
    const int y = m_y / BOSS_GRANULAR_FACTOR;
    const int w = m_hitbox.width / BOSS_GRANULAR_FACTOR;
    const int h = m_hitbox.height / BOSS_GRANULAR_FACTOR;
    for (int ay = 0; ay < h; ++ay)
    {
        for (int ax = 0; ax < w; ++ax)
        {
            const auto c = map.at(x + ax, y + ay);
            const TileDef &def = getTileDef(c);
            if (def.type == TYPE_PLAYER)
                return true;
        }
    }
    return false;
}

bool CBoss::isSolid(const uint8_t c) const
{
    const TileDef &def = getTileDef(c);
    return def.type != TYPE_BACKGROUND && def.type != TYPE_PLAYER;
}

bool CBoss::canMove(const JoyAim aim) const
{
    const CMap &map = CGame::getMap();
    const int x = m_x / BOSS_GRANULAR_FACTOR;
    const int y = m_y / BOSS_GRANULAR_FACTOR;
    const int w = m_hitbox.width / BOSS_GRANULAR_FACTOR;
    const int h = m_hitbox.height / BOSS_GRANULAR_FACTOR;
    if (aim == JoyAim::AIM_UP)
    {
        if (m_y & 1)
            return true;
        if (y == 0)
            return false;
        for (int ax = 0; ax < w; ++ax)
        {
            const auto c = map.at(x + ax, y - 1);
            if (isSolid(c))
                return false;
        }
    }
    else if (aim == JoyAim::AIM_DOWN)
    {
        if (m_y & 1)
            return true;
        if (y + h >= map.hei() * BOSS_GRANULAR_FACTOR)
            return false;
        for (int ax = 0; ax < w; ++ax)
        {
            const auto c = map.at(x + ax, y + h);
            if (isSolid(c))
                return false;
        }
    }
    else if (aim == JoyAim::AIM_LEFT)
    {
        if (m_x & 1)
            return true;
        if (x == 0)
            return false;
        for (int ay = 0; ay < h; ++ay)
        {
            const auto c = map.at(x - 1, y + ay);
            if (isSolid(c))
                return false;
        }
    }
    else if (aim == JoyAim::AIM_RIGHT)
    {
        if (m_x & 1)
            return true;
        if (x + w >= map.len() * BOSS_GRANULAR_FACTOR)
            return false;
        for (int ay = 0; ay < h; ++ay)
        {
            const auto c = map.at(x + w, y + ay);
            if (isSolid(c))
                return false;
        }
    }
    else
    {
        LOGW("invalid aim: %.2x on %d", aim, __LINE__);
        return false;
    }
    return true;
}

void CBoss::move(const JoyAim aim)
{
    switch (aim)
    {
    case JoyAim::AIM_UP:
        --m_y;
        break;
    case JoyAim::AIM_DOWN:
        ++m_y;
        break;
    case JoyAim::AIM_LEFT:
        --m_x;
        break;
    case JoyAim::AIM_RIGHT:
        ++m_x;
        break;
    default:
        LOGE("invalid aim: %.2x", aim);
    }
}

/**
 * @brief Calculate the distance between two actors
 *
 * @param actor
 * @return int
 */
int CBoss::distance(const CActor &actor) const
{
    int dx = std::abs(actor.m_x - m_x / BOSS_GRANULAR_FACTOR);
    int dy = std::abs(actor.m_y - m_y / BOSS_GRANULAR_FACTOR);
    return std::sqrt(dx * dx + dy * dy);
}

void CBoss::move(const Pos pos)
{
    move(pos.x, pos.y);
}