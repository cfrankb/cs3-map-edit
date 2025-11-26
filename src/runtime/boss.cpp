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
#include "randomz.h"
#include "ai_path.h"
#include "shared/IFile.h"
#include "bossdata.h"
#include "filemacros.h"
#include "map.h"
#include "tilesdefs.h"

namespace BossPrivate
{
    constexpr int16_t MAX_POS = 512;
    constexpr int MAX_HP = 4096;
    constexpr int MAX_SPEED = 10;
    constexpr int MAX_FRAME = 16;
};

using namespace BossPrivate;

CBoss::CBoss(const int16_t x, const int16_t y, const bossData_t *data) : m_bossData(data)
{
    m_x = x;
    m_y = y;
    m_speed = data->speed;
    m_state = Patrol;
    m_framePtr = 0;
    m_hp = maxHp(); // data->hp;
    setSolidOperator();
}

bool CBoss::isSolid(const Pos &pos) const
{
    CMap &map = CGame::getMap();
    const auto c = map.at(pos.x, pos.y);
    const TileDef &def = getTileDef(c);
    return def.type != TYPE_BACKGROUND && def.type != TYPE_PLAYER;
}

bool CBoss::isGhostBlocked(const Pos &pos) const
{
    CMap &map = CGame::getMap();
    const auto c = map.at(pos.x, pos.y);
    const TileDef &def = getTileDef(c);
    return def.type == TYPE_SWAMP || def.type == TYPE_ICECUBE || c == TILES_WALLS93_3;
}

std::vector<HitResult> CBoss::testHitbox2(const CMap &map,
                                          hitboxTestCallback_t testCallback,
                                          hitboxActionCallback_t actionCallback) const
{
    std::vector<HitResult> results;

    // Build hitboxes in HALF-TILE units ===
    std::vector<hitbox_t> hitboxes; // now in half-tiles

    // Primary
    hitboxes.push_back({m_x,
                        m_y,
                        m_bossData->hitbox.width,
                        m_bossData->hitbox.height,
                        static_cast<int>(BossData::HitBoxType::MAIN)});

    // Secondary (frame-specific)
    const sprite_hitbox_t *hbData = getHitboxes(m_bossData->sheet, currentFrame());
    for (int i = 0; hbData && i < hbData->count; ++i)
    {
        const auto &c = hbData->hitboxes[i];
        hitboxes.push_back({m_x + c.x - m_bossData->hitbox.x,
                            m_y + c.y - m_bossData->hitbox.y,
                            c.width,
                            c.height,
                            c.type});
    }

    // === 2. For each hitbox, scan WORLD TILES it covers ===
    for (const auto &hb : hitboxes)
    {
        const int left = hb.x;
        const int top = hb.y;
        const int right = left + hb.width - 1;
        const int bottom = top + hb.height - 1;

        const int wx1 = left >> 1;
        const int wy1 = top >> 1;
        const int wx2 = right >> 1;
        const int wy2 = bottom >> 1;

        BossData::HitBoxType type = static_cast<BossData::HitBoxType>(hb.type);

        for (int wy = wy1; wy <= wy2; ++wy)
        {
            for (int wx = wx1; wx <= wx2; ++wx)
            {
                // Out-of-map?
                if (!map.isValid(wx, wy))
                    continue;

                Pos pos{static_cast<int16_t>(wx), static_cast<int16_t>(wy)};

                // Does this world tile actually overlap the hitbox?
                // (Optional: skip if no overlap — but usually yes)
                const int tileLeft = wx << 1; // wx * 2
                const int tileRight = tileLeft + 1;
                const int tileTop = wy << 1;
                const int tileBottom = tileTop + 1;

                if (right < tileLeft || left > tileRight ||
                    bottom < tileTop || top > tileBottom)
                    continue; // no overlap

                // Test callback
                if (testCallback && !testCallback(pos, type))
                    continue;

                // HIT!
                HitResult r{pos, type};
                if (actionCallback)
                    actionCallback(r);
                results.push_back(r);
            }
        }
    }

    return results;
}

std::vector<HitResult> CBoss::testHitbox1(const CMap &map,
                                          hitboxTestCallback_t testCallback,
                                          hitboxActionCallback_t actionCallback) const
{
    std::vector<HitResult> results;

    // Primary hitbox (half-tiles)
    const auto &hbMain = m_bossData->hitbox;
    std::vector<hitbox_t> hitboxes;
    hitboxes.push_back({m_x, m_y, // Keep half-tile coords
                        hbMain.width, hbMain.height,
                        static_cast<int>(BossData::HitBoxType::MAIN)});

    // Secondary hitboxes (relative to main, half-tiles)
    const sprite_hitbox_t *hbData = getHitboxes(m_bossData->sheet, currentFrame());
    for (int i = 0; hbData != nullptr && i < hbData->count; ++i)
    {
        const hitbox_t &c = hbData->hitboxes[i];
        const hitbox_t hb{
            m_x + c.x - hbMain.x,
            m_y + c.y - hbMain.y,
            c.width,
            c.height,
            c.type};
        hitboxes.push_back(hb);
    }

    // Test each hitbox
    for (const auto &hb : hitboxes)
    {
        BossData::HitBoxType hbType = static_cast<BossData::HitBoxType>(hb.type);

        // Compute world tile bounds (half-tiles to full tiles)
        const int left = hb.x;               // Half-tile x
        const int right = hb.x + hb.width;   // Half-tile x+width
        const int top = hb.y;                // Half-tile y
        const int bottom = hb.y + hb.height; // Half-tile y+height

        // World tile range (inclusive)
        const int x_start = left / BOSS_GRANULAR_FACTOR;       // floor(left/2)
        const int x_end = (right - 1) / BOSS_GRANULAR_FACTOR;  // floor((right-1)/2)
        const int y_start = top / BOSS_GRANULAR_FACTOR;        // floor(top/2)
        const int y_end = (bottom - 1) / BOSS_GRANULAR_FACTOR; // floor((bottom-1)/2)

        // Scan all world tiles in range
        for (int wy = y_start; wy <= y_end; ++wy)
        {
            for (int wx = x_start; wx <= x_end; ++wx)
            {
                // Out-of-map check
                if (!map.isValid(wx, wy))
                {
                    continue;
                }

                const Pos pos{static_cast<int16_t>(wx), static_cast<int16_t>(wy)};

                // Test with type
                if (testCallback && !testCallback(pos, hbType))
                {
                    continue;
                }

                // Collect hit
                HitResult result{pos, hbType};
                if (actionCallback)
                {
                    actionCallback(result);
                }
                results.emplace_back(std::move(result));
            }
        }
    }

    return results;
}

bool CBoss::canMove(const JoyAim aim) const
{
    const CMap &map = CGame::getMap();
    const int mapLen = map.width();
    const int mapHei = map.height();
    const int x = m_x / BOSS_GRANULAR_FACTOR;
    const int y = m_y / BOSS_GRANULAR_FACTOR;
    const int w = m_bossData->hitbox.width / BOSS_GRANULAR_FACTOR;
    const int h = m_bossData->hitbox.height / BOSS_GRANULAR_FACTOR;
    const int maxX = mapLen * BOSS_GRANULAR_FACTOR;
    const int maxY = mapHei * BOSS_GRANULAR_FACTOR;

    // Check if move stays within same 8x8 grid cell and map bounds
    int next_x = m_x;
    int next_y = m_y;
    switch (aim)
    {
    case JoyAim::AIM_UP:
        --next_y;
        break;
    case JoyAim::AIM_DOWN:
        ++next_y;
        break;
    case JoyAim::AIM_LEFT:
        --next_x;
        break;
    case JoyAim::AIM_RIGHT:
        ++next_x;
        break;
    default:
        LOGW("invalid aim: %.2x on %d", aim, __LINE__);
        return false;
    }
    if (next_x / BOSS_GRANULAR_FACTOR == m_x / BOSS_GRANULAR_FACTOR &&
        next_y / BOSS_GRANULAR_FACTOR == m_y / BOSS_GRANULAR_FACTOR &&
        next_x >= 0 && next_x + m_bossData->hitbox.width <= maxX &&
        next_y >= 0 && next_y + m_bossData->hitbox.height <= maxY)
    {
        return true; // Sub-tile move within same 8x8 cell and map bounds
    }

    // Check collisions for new 8x8 grid cell
    switch (aim)
    {
    case JoyAim::AIM_UP:
        if (next_y < 0)
            return false;
        for (int ax = x; ax < x + w; ++ax)
        {
            if (ax < 0 || ax >= mapLen)
                continue;
            const Pos pos{static_cast<int16_t>(ax), static_cast<int16_t>(y - 1)};
            if ((this->*m_solidCheck)(pos))
                return false;
        }
        break;

    case JoyAim::AIM_DOWN:
        if (next_y + m_bossData->hitbox.height > maxY)
            return false;
        for (int ax = x; ax < x + w; ++ax)
        {
            if (ax < 0 || ax >= mapLen)
                continue;
            const Pos pos{static_cast<int16_t>(ax), static_cast<int16_t>(y + h)};
            if ((this->*m_solidCheck)(pos))
                return false;
        }
        break;

    case JoyAim::AIM_LEFT:
        if (next_x < 0)
            return false;
        for (int ay = y; ay < y + h; ++ay)
        {
            if (ay < 0 || ay >= mapHei)
                continue;
            const Pos pos{static_cast<int16_t>(x - 1), static_cast<int16_t>(ay)};
            if ((this->*m_solidCheck)(pos))
                return false;
        }
        break;

    case JoyAim::AIM_RIGHT:
        if (next_x + m_bossData->hitbox.width > maxX)
            return false;
        for (int ay = y; ay < y + h; ++ay)
        {
            if (ay < 0 || ay >= mapHei)
                continue;
            const Pos pos{static_cast<int16_t>(x + w), static_cast<int16_t>(ay)};
            if ((this->*m_solidCheck)(pos))
                return false;
        }
        break;
    default:
        LOGE("invalid aim: %.2x on line %d", aim, __LINE__);
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
        return;
    }
    m_aim = aim;
}

/**
 * @brief Calculate the distance between the boss and another actor
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

const boss_seq_t *CBoss::getCurrentSeq() const
{
    switch (m_state)
    {
    case BossState::Patrol:
        return &m_bossData->idle;

    case BossState::Chase:
        return &m_bossData->moving;

    case BossState::Attack:
        return &m_bossData->attack;

    case BossState::Hurt:
        return &m_bossData->hurt;

    case BossState::Death:
        return &m_bossData->death;

    default:
        return nullptr;
    }
}

void CBoss::animate()
{
    const boss_seq_t *seq = getCurrentSeq();
    if (seq == nullptr)
    {
        m_framePtr = 0;
        return;
    }

    int maxFrames = seq->lenght;
    // sanity check
    if (maxFrames == 0)
    {
        m_framePtr = 0;
        return;
    }

    ++m_framePtr;
    if (m_framePtr >= maxFrames)
    {
        m_framePtr = 0;
        if (m_state == BossState::Hurt)
            m_state = BossState::Patrol;
        else if (m_state == BossState::Attack)
            m_state = BossState::Chase;
        else if (m_state == BossState::Death)
            m_state = BossState::Hidden;
    }
}

int CBoss::currentFrame() const
{
    const boss_seq_t *seq = getCurrentSeq();
    if (seq == nullptr)
        return 0;
    const int normalizedAim = m_aim % m_bossData->aims;
    const int baseFrame = seq->base + seq->lenght * normalizedAim;
    return baseFrame + m_framePtr;
}

void CBoss::setState(const BossState state)
{
    m_state = state;
    m_framePtr = 0;
};

int CBoss::maxHp() const
{
    auto skill = (int)CGame::getGame()->skill();
    return (int)m_bossData->hp * ((skill * 0.5) + 1);
}

bool CBoss::subtainDamage(const int lostHP)
{
    bool justDied = false;
    m_hp = std::max(m_hp - lostHP, 0);
    if (m_hp == 0)
    {
        if (m_state != Death)
        {
            justDied = true;
            setState(Death);
        }
    }
    else
    {
        setState(Hurt);
    }
    return justDied;
}

int CBoss::damage(const BossData::HitBoxType hbType) const
{
    switch (hbType)
    {
    case BossData::HitBoxType::MAIN:
        return m_bossData->damage;
    case BossData::HitBoxType::ATTACK:
        return m_bossData->damage_attack;
    case BossData::HitBoxType::SPECIAL1:
        return m_bossData->damage_special1;
    case BossData::HitBoxType::SPECIAL2:
        return m_bossData->damage_special2;
    default:
        return m_bossData->damage;
    }
}

const Pos CBoss::toPos(int x, int y)
{
    return Pos{static_cast<int16_t>(x), static_cast<int16_t>(y)};
}

bool CBoss::followPath(const Pos &playerPos, const IPath &astar)
{
    return m_path.followPath(*this, playerPos, astar) == CPath::Result::MoveSuccesful;
}

void CBoss::patrol()
{
    constexpr JoyAim dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
    Random &rng = CGame::getRandom();
    int dir = rng.range(0, 4); // Choose random direction
    JoyAim aim = dirs[dir];
    if (canMove(aim))
    {
        move(aim);
    }
}

bool CBoss::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == IFILE_OK;
    };

    auto checkBound = [](auto a, auto b)
    {
        using T = decltype(a);

        if constexpr (std::is_same_v<T, size_t>)
        {
            if (a > b)
            {
                LOGE("%s: %lu outside expected -- expected < %lu", _S(a), a, b);
                return false;
            }
        }
        else if constexpr (std::is_same_v<T, uint8_t>)
        {
            if (a > b)
            {
                LOGE("%s: %u outside expected -- expected < %u", _S(a), a, b);
                return false;
            }
        }
        else if constexpr (std::is_integral_v<T>)
        {
            if (a > b || a < 0)
            {
                LOGE("%s: %d outside expected -- expected > 0 && < %d", _S(a), a, b);
                return false;
            }
        }
        else
        {
            static_assert(std::is_integral_v<T>, "checkBound only supports integral types");
        }

        return true;
    };

    constexpr size_t DATA_SIZE = 2;
    uint8_t type = 0;
    _R(&type, sizeof(uint8_t));
    auto data = getBossData(type);
    if (!data)
    {
        LOGE("invalid boss type 0x%.2x", type);
        return false;
    }
    m_bossData = data;

    m_framePtr = 0;
    _R(&m_framePtr, DATA_SIZE);
    checkBound(m_framePtr, MAX_FRAME);
    _R(&m_x, sizeof(m_x)); // uint16_t
    checkBound(m_x, MAX_POS);
    _R(&m_y, sizeof(m_y)); // uint16_t
    checkBound(m_y, MAX_POS);

    m_hp = 0;
    _R(&m_hp, DATA_SIZE);
    checkBound(m_hp, MAX_HP);

    _R(&m_state, sizeof(m_state)); // uint8_t
    checkBound((uint8_t)m_state, (uint8_t)BossState::MAX_STATES);

    m_speed = 0;
    _R(&m_speed, DATA_SIZE);
    checkBound(m_speed, MAX_SPEED);

    _R(&m_aim, sizeof(m_aim)); // uint8_t
    checkBound(static_cast<uint8_t>(m_aim), JoyAim::TOTAL_AIMS);

    // read path
    m_path.read(sfile);

    setSolidOperator();
    return true;
}

bool CBoss::write(IFile &tfile)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    constexpr size_t DATA_SIZE = 2;
    _W(&m_bossData->type, sizeof(uint8_t));
    _W(&m_framePtr, DATA_SIZE);
    _W(&m_x, sizeof(m_x)); // uint16_t
    _W(&m_y, sizeof(m_y)); // uint16_t
    _W(&m_hp, DATA_SIZE);
    _W(&m_state, sizeof(m_state)); // uint8_t
    _W(&m_speed, DATA_SIZE);
    _W(&m_aim, sizeof(m_aim)); // uint8_t

    // write path
    m_path.write(tfile);

    return true;
}

void CBoss::setSolidOperator()
{
    LOGI("set solidOperator for boss %.2x", m_bossData->type);
    if (m_bossData->type == BOSS_MR_DEMON)
        m_solidCheck = &CBoss::isSolid;
    else if (m_bossData->type == BOSS_GHOST)
        m_solidCheck = &CBoss::isGhostBlocked;
    else if (m_bossData->type == BOSS_HARPY)
        m_solidCheck = &CBoss::isSolid;
    else
    {
        LOGW("No custom solidOperator for this boss");
        m_solidCheck = &CBoss::isSolid;
    }
}
