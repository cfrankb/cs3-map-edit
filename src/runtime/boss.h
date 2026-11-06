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
#pragma once

#include <cinttypes>
#include <functional>
#include "rect.h"
#include "joyaim.h"
#include "isprite.h"
#include "bossdata.h"
#include "ai_path.h"

class IPath;
class IFile;
class CActor;
class CPath;
class CBoss;

struct HitResult
{
    Pos pos;                   // World tile (full-tile)
    BossData::HitBoxType type; // e.g., MAIN, VULNERABLE, HURTBOX
};

using hitboxTestCallback_t = std::function<bool(const Pos &, BossData::HitBoxType)>; // Return true to skip/abort this pos
using hitboxActionCallback_t = std::function<void(const HitResult &)>;               // Collect/process each hit
using BossTileCheck = bool (CBoss::*)(const Pos &) const;

class CBoss : public ISprite
{
public:
    enum BossState : uint8_t
    {
        Patrol,
        Chase,
        Attack,
        Flee,
        Hurt,
        Death,
        Hidden,
        MAX_STATES = Hidden
    };
    CBoss(const int16_t x = 0, const int16_t y = 0, const bossData_t *data = nullptr);
    virtual ~CBoss() {};
    inline int16_t x() const override { return m_x; }
    inline int16_t y() const override { return m_y; }
    inline const hitbox_t &hitbox() const { return m_bossData->hitbox; }
    uint8_t type() const override { return m_bossData->type; }
    std::vector<HitResult> testHitbox1(const CMap &map, hitboxTestCallback_t testCallback, hitboxActionCallback_t actionCallback) const;
    std::vector<HitResult> testHitbox2(const CMap &map, hitboxTestCallback_t testCallback, hitboxActionCallback_t actionCallback) const;

    bool isSolid(const Pos &pos) const;
    bool isGhostBlocked(const Pos &pos) const;
    static const Pos toPos(int x, int y);
    bool canMove(const JoyAim aim) const override;
    void move(const JoyAim aim) override;
    int distance(const CActor &actor) const override;
    int speed() const { return m_speed; }
    void setSpeed(int speed) { m_speed = speed; }
    void setHP(const int hp) { m_hp = hp; }
    int hp() const { return m_hp; }
    int maxHp() const;
    bool subtainDamage(const int lostHP);
    bool isHidden() const { return m_state == Hidden; }
    bool isDone() const { return m_state == Hidden; }
    bool isGoal() const { return m_bossData->is_goal; }
    Pos worldPos() const
    {
        constexpr int16_t granular = static_cast<int16_t>(BOSS_GRANULAR_FACTOR);
        return {
            static_cast<int16_t>(m_x / granular),
            static_cast<int16_t>(m_y / granular)};
    }

    inline void move(const int16_t x, const int16_t y) override
    {
        m_x = x;
        m_y = y;
    }
    void move(const Pos pos) override;
    inline BossState state() const
    {
        return m_state;
    }
    void setState(const BossState state);
    inline int16_t getGranularFactor() const override { return BOSS_GRANULAR_FACTOR; };
    inline const Pos pos() const override { return Pos{m_x, m_y}; };
    void animate();
    int currentFrame() const;
    int damage(const BossData::HitBoxType hbType) const;
    int sheet() const { return m_bossData->sheet; }
    int score() const { return m_bossData->score; }
    const char *name() const { return m_bossData->name; }
    const bossData_t *data() const { return m_bossData; }
    enum : int16_t
    {
        BOSS_GRANULAR_FACTOR = 2,
    };
    bool followPath(const Pos &playerPos, const IPath &astar);
    void patrol();
    bool read(IFile &file);
    bool write(IFile &file);
    void setAim(const JoyAim aim) override { m_aim = aim; };
    JoyAim getAim() const override { return m_aim; }
    bool isBoss() const override { return true; }
    int getTTL() const override { return BossData::NoTTL; };

private:
    const boss_seq_t *getCurrentSeq() const;
    const bossData_t *m_bossData;
    int m_framePtr;
    int16_t m_x;
    int16_t m_y;
    int m_hp;
    BossState m_state;
    int m_speed;
    JoyAim m_aim;
    BossTileCheck m_solidCheck = nullptr;
    void setSolidOperator();
    CPath m_path;
};
