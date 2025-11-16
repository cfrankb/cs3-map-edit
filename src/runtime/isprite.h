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

#pragma once

#include "rect.h"
#include "joyaim.h"
#include "map.h"

#include <cstdint>
class CActor;
class ISprite
{
public:
    virtual ~ISprite() {};
    virtual int16_t x() const = 0;
    virtual int16_t y() const = 0;
    virtual uint8_t type() const = 0;
    virtual bool canMove(const JoyAim aim) const = 0;
    virtual void move(const JoyAim aim) = 0;
    virtual int distance(const CActor &actor) const = 0;
    virtual void move(const int16_t x, const int16_t y) = 0;
    virtual void move(const Pos pos) = 0;
    virtual inline int16_t getGranularFactor() const = 0;
    virtual const Pos pos() const = 0;
    virtual void setAim(const JoyAim aim) = 0;
    virtual JoyAim getAim() const = 0;
    virtual bool isBoss() const = 0;
    virtual int getTTL() const = 0;
};