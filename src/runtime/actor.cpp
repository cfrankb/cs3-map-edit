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
#include <algorithm>

#include "actor.h"
#include "tilesdata.h"
#include "tilesdefs.h"
#include "game.h"
#include "sprtypes.h"
#include "attr.h"
#include <cstdio>
#include <cstdlib> // std::abs(int) (audit L550)
#include <cmath>
#include <stdexcept>
#include <memory>
#include "shared/IFile.h"
#include "logger.h"
#include "bossdata.h"

namespace ActorData
{
    constexpr const JoyAim g_aims[] = {
        AIM_DOWN, AIM_RIGHT, AIM_UP, AIM_LEFT,
        AIM_UP, AIM_LEFT, AIM_DOWN, AIM_RIGHT,
        AIM_RIGHT, AIM_UP, AIM_LEFT, AIM_DOWN,
        AIM_LEFT, AIM_DOWN, AIM_RIGHT, AIM_UP};

    constexpr uint8_t PATH_NOT_DEFINED = 0;
    constexpr uint8_t PATH_DEFINED = 1;
}

using namespace ActorData;

/**
 * @brief Reverse a given direction
 *
 * @param aim
 * @return JoyAim
 */
JoyAim reverseDir(const JoyAim aim)
{
    switch (aim)
    {
    case AIM_UP:
        return AIM_DOWN;
    case AIM_DOWN:
        return AIM_UP;
    case AIM_RIGHT:
        return AIM_LEFT;
    case AIM_LEFT:
        return AIM_RIGHT;
    default:
        return aim;
    }
}

CActor::CActor(const uint8_t x, const uint8_t y, const uint8_t type, const JoyAim aim) : m_path(nullptr)
{
    if (aim >= TOTAL_AIMS && aim != AIM_NONE)
        throw std::invalid_argument("Invalid aim value");
    m_x = x;
    m_y = y;
    m_type = type;
    m_aim = aim;
    m_pu = TILES_BLANK;
    m_algo = BossData::Path::NONE;
    m_frame = 0;
    m_ttl = CActor::NoTTL;
    m_attr = 0;
    m_origin = Pos{x, y};
}

// delegates so both ctors share the aim validation (audit L85-97)
CActor::CActor(const Pos &pos, uint8_t type, JoyAim aim)
    : CActor(static_cast<uint8_t>(pos.x), static_cast<uint8_t>(pos.y), type, aim)
{
}

CActor::CActor(CActor &&other) noexcept
    : m_x(other.m_x),
      m_y(other.m_y),
      m_type(other.m_type),
      m_frame(other.m_frame),
      m_algo(other.m_algo),
      m_aim(other.m_aim),
      m_pu(other.m_pu),
      m_attr(other.m_attr),
      m_ttl(other.m_ttl),
      m_origin(other.m_origin),
      m_path(std::move(other.m_path))
{
    // You can't move ISprite, but its state is already constructed
    // If ISprite has internal state, you may need a reset/init method
    other.m_x = 0;
    other.m_y = 0;
    other.m_type = 0;
    other.m_frame = 0;
    other.m_aim = AIM_UP;
    other.m_pu = 0;
    other.m_algo = BossData::Path::NONE;
    other.m_ttl = CActor::NoTTL;
    other.m_attr = 0;
    other.m_origin = Pos{0, 0};
}

CActor &CActor::operator=(CActor &&other) noexcept
{
    if (this != &other)
    {
        // No ISprite::operator= — skip base move
        m_x = other.m_x;
        m_y = other.m_y;
        m_type = other.m_type;
        m_aim = other.m_aim;
        m_pu = other.m_pu;
        m_frame = other.m_frame;
        m_algo = other.m_algo;
        m_path = std::move(other.m_path);
        m_ttl = other.m_ttl;
        m_attr = other.m_attr;
        m_origin = other.m_origin;

        other.m_x = 0;
        other.m_y = 0;
        other.m_type = 0;
        other.m_frame = 0;
        other.m_aim = AIM_UP;
        other.m_pu = 0;
        other.m_algo = BossData::Path::NONE;
        other.m_ttl = CActor::NoTTL;
        other.m_attr = 0;
        other.m_origin = Pos{0, 0};
    }
    return *this;
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

bool CActor::canMove(const JoyAim aim) const
{
    const CMap &map = CGame::getMap();
    const Pos &pos = Pos{m_x, m_y};
    const Pos &newPos = CGame::translate(pos, aim);
    if (pos.x == newPos.x && pos.y == newPos.y)
    {
        return false;
    }

    const CGame *game = CGame::getGame();
    scan_t result = game->scanPos(newPos, aim);
    if (result.isSolid)
        return false;
    else if ((result.isWater || result.isDeadly) && m_type != TYPE_PLAYER)
        return false;
    if (game->isGranularSolidFromPos(pos, aim))
        return false;

    const uint16_t c = map.at(newPos.x, newPos.y);
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
            def.type == TYPE_CHUTE ||
            def.type == TYPE_KEY ||
            def.type == TYPE_FIRE)
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
    else if (m_type == TYPE_MONSTERV3)
    {
        // The player's tile is stamped on the map at their cell, so without
        // this the A* goal is unreachable and MONSTERV3 never paths to the
        // player (it stood still firing forever). CPath::followPath refuses
        // the actual final step; the AI_STICKY contact damage in
        // handleMonsterV3 is what halts the monster at the player.
        if (def.type == TYPE_PLAYER)
            return true;
    }
    else if (CGame::isMoveableType(m_type) || CGame::isBulletType(m_type))
    {
        if (def.type == TYPE_STOP)
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
    const uint16_t c = map.at(m_x, m_y);
    map.set(m_x, m_y, m_pu);

    const Pos pos = CGame::translate(Pos{m_x, m_y}, aim);
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

uint16_t CActor::getPU() const
{
    return m_pu;
}

/**
 * @brief Set TileID under sprite position
 *
 * @param c
 */

void CActor::setPU(const uint16_t c)
{
    m_pu = c;
}

/**
 * @brief Set Sprite Position
 *
 * Moves the actor only: deliberately does NOT touch the map. Distinct
 * use case from move() -- repositioning (camera placement, actor setup)
 * where the cells must stay put. If a caller needs move()-style cell
 * bookkeeping (the occupied cell holds `m_pu`), it performs that swap
 * itself. Maintainer ruling 2026-11-18.
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
JoyAim CActor::findNextDir(const bool reverse) const
{
    const int aim = m_aim;
    if (aim >= TOTAL_AIMS)
        return AIM_NONE; // AIM_NONE is an inactive-actor placeholder, not a table index
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
    const uint16_t c = tileAt(aim);
    const TileDef &def = getTileDef(c);
    return def.type == TYPE_PLAYER;
}

/**
 * @brief Get tileID at give relative position
 *
 * @param aim
 * @return uint16_t
 */
uint16_t CActor::tileAt(JoyAim aim) const
{
    const CMap &map = CGame::getMap();
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
    m_aim = ::reverseDir(m_aim);
}

bool CActor::readPath(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        if (sfile.read(ptr, size) != IFILE_OK)
        {
            LOGE("CActor::readPath: failed to read %zu bytes at %ld", size, sfile.tell());
            return false;
        }
        return true;
    };
    // read path
    uint8_t pathDefined = 0;
    if (!readfile(&pathDefined, sizeof(pathDefined)))
        return false;
    if (pathDefined != PATH_NOT_DEFINED && pathDefined != PATH_DEFINED)
    {
        // sanity check
        LOGE("invalid value for pathDefined: %.2x", pathDefined);
        return false;
    }
    if (pathDefined == PATH_DEFINED)
    {
        if (!m_path)
            m_path = std::make_unique<CPath>(); // never null: bad_alloc throws
        if (!m_path->read(sfile))
            return false;
    }
    return true;
}

bool CActor::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        if (sfile.read(ptr, size) != IFILE_OK)
        {
            LOGE("CActor::read: failed to read %zu bytes at %ld", size, sfile.tell());
            return false;
        }
        return true;
    };

    return readCommon(readfile) && readPath(sfile);
}

/**
 * @brief Common deserializer for this class
 *
 * @tparam ReadFunc
 * @param readfile
 * @return true
 * @return false
 */
template <typename ReadFunc>
bool CActor::readCommon(ReadFunc readfile)
{
    if (!readfile(&m_x, sizeof(m_x)))
        return false;
    if (!readfile(&m_y, sizeof(m_y)))
        return false;
    if (!readfile(&m_type, sizeof(m_type)))
        return false;
    if (!readfile(&m_aim, sizeof(m_aim)))
        return false;
    // defense in depth (audit L444): the ctor validates aim, and a raw
    // m_aim feeds the g_aims table lookup; AIM_NONE is the only valid
    // out-of-range value (inactive-actor placeholder, saved routinely)
    if (m_aim >= TOTAL_AIMS && m_aim != AIM_NONE)
    {
        LOGE("CActor::read: invalid aim %.2x", m_aim);
        return false;
    }
    if (!readfile(&m_pu, sizeof(m_pu)))
        return false;
    if (!readfile(&m_algo, sizeof(m_algo)))
        return false;
    if (!readfile(&m_frame, sizeof(m_frame)))
        return false;
    if (!readfile(&m_ttl, sizeof(m_ttl)))
        return false;
    if (!readfile(&m_attr, sizeof(m_attr)))
        return false;
    if (!readfile(&m_origin.x, sizeof(m_origin.x)))
        return false;
    if (!readfile(&m_origin.y, sizeof(m_origin.y)))
        return false;
    return true;
}

bool CActor::writePath(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    if (!writefile(m_path ? &PATH_DEFINED : &PATH_NOT_DEFINED, sizeof(PATH_DEFINED)))
        return false;

    if (m_path && !m_path->write(tfile))
        return false;
    return true;
}

bool CActor::write(IFile &tfile) const
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    return writeCommon(writefile) && writePath(tfile);
}

/**
 * @brief common serializer for this class
 *
 * @tparam WriteFunc
 * @param writefile
 * @return true
 * @return false
 */
template <typename WriteFunc>
bool CActor::writeCommon(WriteFunc writefile) const
{
    if (!writefile(&m_x, sizeof(m_x)))
        return false;
    if (!writefile(&m_y, sizeof(m_y)))
        return false;
    if (!writefile(&m_type, sizeof(m_type)))
        return false;
    if (!writefile(&m_aim, sizeof(m_aim)))
        return false;
    if (!writefile(&m_pu, sizeof(m_pu)))
        return false;
    if (!writefile(&m_algo, sizeof(m_algo)))
        return false;
    if (!writefile(&m_frame, sizeof(m_frame)))
        return false;
    if (!writefile(&m_ttl, sizeof(m_ttl)))
        return false;
    if (!writefile(&m_attr, sizeof(m_attr)))
        return false;
    if (!writefile(&m_origin.x, sizeof(m_origin.x)))
        return false;
    if (!writefile(&m_origin.y, sizeof(m_origin.y)))
        return false;
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
    return Pos{static_cast<int16_t>(m_x), static_cast<int16_t>(m_y)};
}

/**
 * @brief Calculate the distance between two actors
 *
 * @param actor
 * @return int
 */
int CActor::distance(const CActor &actor) const
{
    int dx = std::abs(actor.m_x - m_x);
    int dy = std::abs(actor.m_y - m_y);
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Move Sprite to a specfic coordonates
 *
 * @param x
 * @param y
 */
void CActor::move(const int16_t x, const int16_t y)
{
    // coordinates are capped to the representable cell range, never
    // wrapped (maintainer ruling 2026-11-18)
    m_x = static_cast<uint8_t>(std::clamp<int16_t>(x, 0, UINT8_MAX));
    m_y = static_cast<uint8_t>(std::clamp<int16_t>(y, 0, UINT8_MAX));
}

void CActor::move(const Pos pos)
{
    move(pos.x, pos.y);
}

CPath::Result CActor::followPath(const Pos &playerPos)
{
    auto pathAlgo = CPath::getPathAlgo(m_algo);
    if (!pathAlgo)
        return CPath::Result::NotConfigured;
    if (m_path)
    {
        decTTL();
        auto result = m_path->followPath(*this, playerPos, *pathAlgo);
        if (m_ttl == 0 && CGame::isBulletType(m_type))
            m_path.reset(); // replaces delete + nullptr

        return result;
    }
    return CPath::Result::NotConfigured;
}

bool CActor::isFollowingPath()
{
    return m_path != nullptr;
}

bool CActor::startPath(const Pos &playerPos, const uint8_t algo, const int ttl)
{
    m_algo = algo;
    auto pathAlgo = CPath::getPathAlgo(algo);
    if (!pathAlgo)
        return false;
    if (!m_path)
        m_path = std::make_unique<CPath>();
    m_ttl = ttl;
    // Result is not a bool: NoValidPath/NotConfigured are truthy ints
    return m_path->followPath(*this, playerPos, *pathAlgo) == CPath::Result::MoveSuccesful;
}