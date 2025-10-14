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
#include "rect.h"
#include "joyaim.h"
#include "actor.h"
#include "isprite.h"

enum BossState : uint8_t
{
    Patrol,
    Chase,
    Attack,
    Flee,
};

class CBoss : public ISprite
{
public:
    CBoss(const int16_t x, const int16_t y, const Rect hitbox, const uint8_t type);
    virtual ~CBoss() = default;
    inline int16_t x() const override { return m_x; }
    inline int16_t y() const override { return m_y; }
    inline const Rect &hitbox() const { return m_hitbox; }
    uint8_t type() const override { return m_type; }
    bool isPlayerHere() const;
    bool canMove(const JoyAim aim) const override;
    void move(const JoyAim aim) override;
    int distance(const CActor &actor) const override;
    int speed() const { return m_speed; }
    void setSpeed(int speed) { m_speed = speed; }
    void move(const int16_t x, const int16_t y) override
    {
        m_x = x;
        m_y = y;
    }
    void move(const Pos pos) override;
    inline BossState state() const
    {
        return m_state;
    }
    inline void setState(const BossState state)
    {
        m_state = state;
    };
    inline int16_t granularFactor() const override { return BOSS_GRANULAR_FACTOR; };
    inline const Pos pos() const override { return Pos{m_x, m_y}; };

    enum : int16_t
    {
        BOSS_GRANULAR_FACTOR = 2
    };

private:
    bool isSolid(const uint8_t c) const;

    int16_t m_x;
    int16_t m_y;
    uint8_t m_type;
    BossState m_state;
    Rect m_hitbox;
    int m_speed;
};
