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

#include <algorithm>
#include <cstdint>
#include <set>
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
}

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
    CGame *game = CGame::getGame();
    scan_t result = game->scanPos(pos, JoyAim::AIM_NONE);
    CMap &map = CGame::getMap();
    const auto c = map.at(pos.x, pos.y);
    const TileDef &def = getTileDef(c);
    return def.type == TYPE_SWAMP || def.type == TYPE_ICECUBE || c == TILES_WALLS93_3 || result.isWater;
}

std::vector<HitResult> CBoss::testHitbox1(const CMap &map,
                                          const hitboxTestCallback_t &testCallback,
                                          const hitboxActionCallback_t &actionCallback) const
{
    std::vector<HitResult> results;

    // Primary hitbox (half-tiles)
    const auto &hbMain = m_bossData->hitbox;
    std::vector<hitbox_t> hitboxes;
    hitboxes.push_back({m_x, m_y, // Keep half-tile coords
                        hbMain.width, hbMain.height,
                        static_cast<int>(BossData::HitBoxType::MAIN)});

    // Secondary hitboxes (relative to main, half-tiles). On a mirrored
    // aim the frame is the twin direction's, so reflect each authored box
    // in boss space: x -> -x - width (plan boss-aims3-mirror §2).
    const bool mirrored = isMirrored();
    const sprite_hitbox_t *hbData = getHitboxes(m_bossData->sheet, currentFrame());
    for (int i = 0; hbData != nullptr && i < hbData->count; ++i)
    {
        const hitbox_t &c = hbData->hitboxes[i];
        const hitbox_t hb{
            mirrored ? m_x - (c.x - hbMain.x) - c.width : m_x + c.x - hbMain.x,
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

        // World tile range (inclusive). C++ division truncates toward zero;
        // hitbox half-tile coordinates are non-negative by data contract
        // (maintainer ruling 2026-11-18), so trunc == floor here.
        const int x_start = left / BOSS_GRANULAR_FACTOR;
        const int x_end = (right - 1) / BOSS_GRANULAR_FACTOR;
        const int y_start = top / BOSS_GRANULAR_FACTOR;
        const int y_end = (bottom - 1) / BOSS_GRANULAR_FACTOR;

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
    const int w = m_bossData->hitbox.width;  // half-tiles
    const int h = m_bossData->hitbox.height; // half-tiles
    const int maxX = map.width() * BOSS_GRANULAR_FACTOR;
    const int maxY = map.height() * BOSS_GRANULAR_FACTOR;

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
    if (next_x < 0 || next_y < 0 ||
        next_x + w > maxX || next_y + h > maxY)
        return false;

    // The hitbox spans several world tiles; every tile it would *newly*
    // cover must be non-solid. Checking only the anchor tile's row/column
    // let a half-tile step slide the hitbox's far edge into a blocking
    // tile (e.g. a static ice cube the boss then collided with).
    const int nx1 = next_x / BOSS_GRANULAR_FACTOR;
    const int nx2 = (next_x + w - 1) / BOSS_GRANULAR_FACTOR;
    const int ny1 = next_y / BOSS_GRANULAR_FACTOR;
    const int ny2 = (next_y + h - 1) / BOSS_GRANULAR_FACTOR;
    const int ox1 = m_x / BOSS_GRANULAR_FACTOR;
    const int ox2 = (m_x + w - 1) / BOSS_GRANULAR_FACTOR;
    const int oy1 = m_y / BOSS_GRANULAR_FACTOR;
    const int oy2 = (m_y + h - 1) / BOSS_GRANULAR_FACTOR;

    for (int ty = ny1; ty <= ny2; ++ty)
    {
        for (int tx = nx1; tx <= nx2; ++tx)
        {
            if (tx >= ox1 && tx <= ox2 && ty >= oy1 && ty <= oy2)
                continue; // already overlapped before the move
            if (!map.isValid(tx, ty))
                return false;
            const Pos pos{static_cast<int16_t>(tx), static_cast<int16_t>(ty)};
            if ((this->*m_solidCheck)(pos))
                return false;
        }
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

    const int maxFrames = seq->lenght;
    if (maxFrames <= 0)
    {
        // zero-length seq: no advance, and no Hurt->Patrol-style transitions
        // either (the old 0 >= 0 fallthrough ran them; audit 284-301)
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
    // aims are validated/warned at load (bossdata build); clamp kept here
    // as defense-in-depth so the modulo can never divide by zero.
    const int aims = m_bossData->aims > 0 ? m_bossData->aims : 1;
    int normalizedAim = 0;
    if (aims == 3)
    {
        // aims-3 layout (plan docs/plans/boss-aims3-mirror-plan.md §2):
        // slots are UP=0, DOWN=1 and ONE horizontal block at 2 shared by
        // LEFT and RIGHT; data->mirror says which of them is the flipped
        // copy (see isMirrored()). AIM_NONE (never-moved/just-restored)
        // and any out-of-range aim normalise to UP, the slot-0 convention.
        normalizedAim = (m_aim == AIM_UP || m_aim == AIM_DOWN) ? static_cast<int>(m_aim)
                      : (m_aim < TOTAL_AIMS ? 2 : 0);
    }
    else
    {
        normalizedAim = m_aim % aims;
    }
    const int baseFrame = seq->base + seq->lenght * normalizedAim;
    return baseFrame + m_framePtr;
}

void CBoss::warnAsymAnchor(const int frameWidthPx, const int grid) const
{
    // the primary hitbox doubles as the sprite draw anchor; when an aims-3
    // mirror draws the shared frame flipped, an anchor that is not
    // horizontally centred (hb.x == frameW - hb.x - hb.width) shifts the
    // mirrored sprite. warn once per boss type (plan §7).
    static std::set<uint8_t> warned;
    if (m_bossData == nullptr || warned.count(m_bossData->type))
        return;
    warned.insert(m_bossData->type);
    const int frameWHalf = frameWidthPx / grid;
    const hitbox_t &hb = m_bossData->hitbox;
    if (hb.x != frameWHalf - hb.x - hb.width)
    {
        LOGW("boss '%s' aims-3 anchor hb.x=%d w=%d frame=%d half-tiles: "
             "mirrored draw may be off-centre\n",
             m_bossData->name, hb.x, hb.width, frameWHalf);
    }
}

void CBoss::setState(const BossState state)
{
    m_state = state;
    m_framePtr = 0;
    const boss_seq_t *seq = getCurrentSeq();
    if (seq != nullptr && seq->lenght == 0)
        m_state = Hidden;
}

int CBoss::maxHp() const
{
    const auto skill = (int)CGame::getGame()->skill();
    return m_bossData->hp * (2 + skill) / 2; // same values as hp*(skill*0.5+1), no fp (audit 323-327)
}

bool CBoss::sustainDamage(const int lostHP)
{
    bool justDied = false;
    m_hp = std::max(m_hp - std::max(lostHP, 0), 0); // a negative lostHP must not heal past maxHp (audit 332)
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
    // clamp instead of silently wrapping through int16_t (audit 365-368)
    return Pos{static_cast<int16_t>(std::clamp<int>(x, INT16_MIN, INT16_MAX)),
               static_cast<int16_t>(std::clamp<int>(y, INT16_MIN, INT16_MAX))};
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
    // BUG-C6 follow-up: like the ctor, restored bosses need their tile-check
    // operator; without it isSolid callers deref a null member pointer.
    setSolidOperator();

    m_framePtr = 0;
    uint16_t u16 = 0;
    _R(&u16, DATA_SIZE);
    m_framePtr = u16;
    if (!checkBound(m_framePtr, MAX_FRAME))
        return false;
    _R(&m_x, sizeof(m_x)); // int16_t
    if (!checkBound(m_x, MAX_POS))
        return false;
    _R(&m_y, sizeof(m_y)); // int16_t
    if (!checkBound(m_y, MAX_POS))
        return false;

    m_hp = 0;
    _R(&u16, DATA_SIZE);
    m_hp = u16;
    if (!checkBound(m_hp, MAX_HP))
        return false;

    _R(&m_state, sizeof(m_state)); // uint8_t
    if (!checkBound((uint8_t)m_state, (uint8_t)BossState::MAX_STATES))
        return false;

    m_speed = 0;
    _R(&u16, DATA_SIZE);
    m_speed = u16;
    if (!checkBound(m_speed, MAX_SPEED))
        return false;

    _R(&m_aim, sizeof(m_aim)); // uint8_t
    // AIM_NONE is a legal on-disk value (a boss restored before its first
    // move); currentFrame() folds it into range with the aims modulo.
    if (m_aim >= JoyAim::TOTAL_AIMS && m_aim != JoyAim::AIM_NONE)
    {
        LOGE("boss aim %u outside expected -- expected < %u", m_aim, (unsigned)JoyAim::TOTAL_AIMS);
        return false;
    }

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
    uint16_t u16 = 0;
    _W(&m_bossData->type, sizeof(uint8_t));
    u16 = static_cast<uint16_t>(m_framePtr);
    _W(&u16, DATA_SIZE);
    _W(&m_x, sizeof(m_x)); // uint16_t
    _W(&m_y, sizeof(m_y)); // uint16_t
    u16 = static_cast<uint16_t>(m_hp);
    _W(&u16, DATA_SIZE);
    _W(&m_state, sizeof(m_state)); // uint8_t
    u16 = static_cast<uint16_t>(m_speed);
    _W(&u16, DATA_SIZE);
    _W(&m_aim, sizeof(m_aim)); // uint8_t

    // write path
    m_path.write(tfile);

    return true;
}

// DO NOT collapse this per-type branch chain: one named branch per boss
// type is the intended extension point for wiring a different solid-check
// operator to a specific type when new ones are added
// (maintainer ruling 2026-11-18; see docs/audits/boss.cpp.audit.md).
void CBoss::setSolidOperator()
{
    LOGI("set solidOperator for boss %.2x", m_bossData->type);
    if (m_bossData->type == BOSS_MR_DEMON)
        m_solidCheck = &CBoss::isSolid;
    else if (m_bossData->type == BOSS_GHOST)
        m_solidCheck = &CBoss::isGhostBlocked;
    else if (m_bossData->type == BOSS_HARPY)
        m_solidCheck = &CBoss::isSolid;
    else if (m_bossData->type == BOSS_BEHOLDER)
        m_solidCheck = &CBoss::isSolid;
    else
    {
        LOGW("No custom solidOperator for this boss");
        m_solidCheck = &CBoss::isSolid;
    }
}
