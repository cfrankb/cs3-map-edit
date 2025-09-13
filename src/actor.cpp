/*
    cs3-runtime-sdl
    Copyright (C) 2024  Francois Blanchette

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
#include "actor.h"
#include "tilesdata.h"
#include "game.h"
#include "sprtypes.h"
#include "attr.h"
#include <cstdio>

const JoyAim g_aims[] = {
    AIM_DOWN, AIM_RIGHT, AIM_UP, AIM_LEFT,
    AIM_UP, AIM_LEFT, AIM_DOWN, AIM_RIGHT,
    AIM_RIGHT, AIM_UP, AIM_LEFT, AIM_DOWN,
    AIM_LEFT, AIM_DOWN, AIM_RIGHT, AIM_UP};

/**
 * @brief Reverse a given direction
 *
 * @param aim
 * @return JoyAim
 */
JoyAim reverseDir(const JoyAim aim)
{
    if (aim == AIM_UP)
        return AIM_DOWN;
    else if (aim == AIM_DOWN)
        return AIM_UP;
    else if (aim == AIM_RIGHT)
        return AIM_LEFT;
    else if (aim == AIM_LEFT)
        return AIM_RIGHT;
    else
        return aim;
}

CActor::CActor(const uint8_t x, const uint8_t y, const uint8_t type, const JoyAim aim)
{
    m_x = x;
    m_y = y;
    m_type = type;
    m_aim = aim;
    m_pu = TILES_BLANK;
}

CActor::CActor(const Pos &pos, uint8_t type, JoyAim aim)
{
    m_x = pos.x;
    m_y = pos.y;
    m_type = type;
    m_aim = aim;
    m_pu = TILES_BLANK;
}

CActor::~CActor()
{
}

/**
 * @brief Can Sprite Move in given direction
 *
 * @param aim
 * @return true
 * @return false
 */

bool CActor::canMove(const JoyAim aim)
{
    CMap &map = CGame::getMap();
    const Pos &pos = Pos{m_x, m_y};
    const Pos &newPos = CGame::translate(pos, aim);
    if (pos.x == newPos.x && pos.y == newPos.y)
    {
        return false;
    }

    const uint8_t c = map.at(newPos.x, newPos.y);
    const TileDef &def = getTileDef(c);
    if (def.type == TYPE_BACKGROUND)
    {
        return true;
    }
    else if (m_type == TYPE_PLAYER)
    {
        if (def.type == TYPE_SWAMP ||
            def.type == TYPE_PICKUP ||
            def.type == TYPE_DIAMOND ||
            def.type == TYPE_STOP ||
            def.type == TYPE_KEY)
        {
            return true;
        }
        else if (def.type == TYPE_DOOR)
        {
            return CGame::hasKey(c + 1);
        }
    }
    else if (RANGE(m_type, ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX))
    {
        if (def.type == TYPE_PLAYER)
            return true;
    }
    return false;
}

/**
 * @brief Move Sprite in given Direction
 *
 * @param aim
 */

void CActor::move(const JoyAim aim)
{
    CMap &map = CGame::getMap();
    uint8_t c = map.at(m_x, m_y);
    map.set(m_x, m_y, m_pu);

    Pos pos{m_x, m_y};
    pos = CGame::translate(pos, aim);
    m_x = pos.x;
    m_y = pos.y;

    m_pu = map.at(m_x, m_y);
    map.set(m_x, m_y, c);
    m_aim = aim;
}

/**
 * @brief Get TileID under sprite position
 *
 * @return uint8_t
 */

uint8_t CActor::getPU() const
{
    return m_pu;
}

/**
 * @brief Set TileID under sprite position
 *
 * @param c
 */

void CActor::setPU(const uint8_t c)
{
    m_pu = c;
}

/**
 * @brief Set Sprite Position
 *
 * @param pos
 */

void CActor::setPos(const Pos &pos)
{
    m_x = pos.x;
    m_y = pos.y;
}

/**
 * @brief Find Next Director (monsters)
 *
 * @param reverse, flip search order
 * @return JoyAim
 */
JoyAim CActor::findNextDir(const bool reverse)
{
    const int aim = m_aim;
    int i = TOTAL_AIMS - 1;
    while (i >= 0)
    {
        const bool isRevered = (i & 1) && reverse;
        JoyAim newAim = g_aims[aim * TOTAL_AIMS + i];
        if (isRevered)
        {
            newAim = ::reverseDir(newAim);
        }
        if (canMove(newAim))
        {
            return newAim;
        }
        --i;
    }
    return AIM_NONE;
}

/**
 * @brief Get Sprite Current Direction
 *
 * @return JoyAim
 */
JoyAim CActor::getAim() const
{
    return m_aim;
}

/**
 * @brief Set Sprite Current Direction
 *
 * @param aim
 */
void CActor::setAim(const JoyAim aim)
{
    m_aim = aim;
}

/**
 * @brief Is Player present at given relative postion
 *
 * @param aim
 * @return true
 * @return false
 */

bool CActor::isPlayerThere(JoyAim aim) const
{
    const uint8_t c = tileAt(aim);
    const TileDef &def = getTileDef(c);
    return def.type == TYPE_PLAYER;
}

/**
 * @brief Get tileID at give relative position
 *
 * @param aim
 * @return uint8_t
 */
uint8_t CActor::tileAt(JoyAim aim) const
{
    CMap &map = CGame::getMap();
    const Pos &p = CGame::translate(Pos{m_x, m_y}, aim);
    return map.at(p.x, p.y);
}

/**
 * @brief Change Sprite Type
 *
 * @param type
 */
void CActor::setType(const uint8_t type)
{
    m_type = type;
}

/**
 * @brief Check if Sprite Within this range of Map Coordonates
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @return true
 * @return false
 */
bool CActor::isWithin(const int x1, const int y1, const int x2, const int y2) const
{
    return (m_x >= x1) && (m_x < x2) && (m_y >= y1) && (m_y < y2);
}

/**
 * @brief Reverse Sprite Direction
 *
 */
void CActor::reverveDir()
{
    m_aim ^= 1;
}

/**
 * @brief Deserialize Sprite from disk
 *
 * @param sfile
 * @return true
 * @return false
 */

bool CActor::read(FILE *sfile)
{
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    readfile(&m_x, sizeof(m_x));
    readfile(&m_y, sizeof(m_y));
    readfile(&m_type, sizeof(m_type));
    readfile(&m_aim, sizeof(m_aim));
    readfile(&m_pu, sizeof(m_pu));
    return true;
}

/**
 * @brief Serialize Sprite to disk
 *
 * @param tfile
 * @return true
 * @return false
 */
bool CActor::write(FILE *tfile)
{
    auto writefile = [tfile](auto ptr, auto size)
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };
    writefile(&m_x, sizeof(m_x));
    writefile(&m_y, sizeof(m_y));
    writefile(&m_type, sizeof(m_type));
    writefile(&m_aim, sizeof(m_aim));
    writefile(&m_pu, sizeof(m_pu));
    return true;
}

/**
 * @brief Reverse Aim
 *
 * @param aim
 * @param i     bitmask
 * @return JoyAim
 */
JoyAim operator^=(JoyAim &aim, int i)
{
    return static_cast<JoyAim>(static_cast<int>(aim) ^ i);
}

/**
 * @brief  Return Sprite Positon
 *
 * @return const Pos
 */
const Pos CActor::pos() const
{
    return Pos{.x = m_x, .y = m_y};
}

/**
 * @brief Calculate the distance between two actors
 *
 * @param actor
 * @return int
 */
int CActor::distance(const CActor &actor)
{
    int dx = std::abs(actor.m_x - m_x);
    int dy = std::abs(actor.m_y - m_y);
    return std::max(dx, dy);
}
