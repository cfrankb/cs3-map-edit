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
#pragma once
#include <stdio.h>
#include <cstdint>
#include "map.h"

enum JoyAim : uint8_t
{
    AIM_UP = 0,
    AIM_DOWN = 1,
    AIM_LEFT = 2,
    AIM_RIGHT = 3,
    TOTAL_AIMS = 4,
    AIM_NONE = 0xff,
};

JoyAim operator^=(JoyAim &aim, int i);
JoyAim reverseDir(const JoyAim aim);

class CActor
{

public:
    CActor(const uint8_t x = 0, const uint8_t y = 0, const uint8_t type = 0, const JoyAim aim = AIM_UP);
    CActor(const Pos &pos, uint8_t type = 0, JoyAim aim = AIM_UP);
    ~CActor();

    bool canMove(const JoyAim aim);
    void move(const JoyAim aim);
    inline uint8_t getX() const
    {
        return m_x;
    }
    inline uint8_t getY() const
    {
        return m_y;
    }
    uint8_t getPU() const;
    void setPU(const uint8_t c);
    void setPos(const Pos &pos);
    JoyAim getAim() const;
    void setAim(const JoyAim aim);
    JoyAim findNextDir(const bool reverse = false);
    bool isPlayerThere(JoyAim aim) const;
    uint8_t tileAt(JoyAim aim) const;
    void setType(const uint8_t type);
    bool isWithin(const int x1, const int y1, const int x2, const int y2) const;
    bool read(FILE *sfile);
    bool write(FILE *tfile);
    void reverveDir();
    const Pos pos() const;
    int distance(const CActor &actor);

private:
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_type;
    JoyAim m_aim;
    uint8_t m_pu;

    friend class CGame;
};
