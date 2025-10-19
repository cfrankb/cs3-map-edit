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
#include <cstdint>
#include "map.h"
#include "joyaim.h"
#include "isprite.h"

class IFile;
class CActor : public ISprite
{

public:
    CActor(const uint8_t x = 0, const uint8_t y = 0, const uint8_t type = 0, const JoyAim aim = AIM_UP);
    CActor(const Pos &pos, uint8_t type = 0, JoyAim aim = AIM_UP);
    ~CActor();

    bool canMove(const JoyAim aim) const override;
    void move(const JoyAim aim) override;
    inline int16_t x() const override
    {
        return static_cast<int16_t>(m_x);
    }
    inline int16_t y() const override
    {
        return static_cast<int16_t>(m_y);
    }
    inline uint8_t type() const override
    {
        return m_type;
    }
    uint8_t getPU() const;
    void setPU(const uint8_t c);
    void setPos(const Pos &pos);
    JoyAim getAim() const;
    void setAim(const JoyAim aim);
    JoyAim findNextDir(const bool reverse = false) const;
    bool isPlayerThere(JoyAim aim) const;
    uint8_t tileAt(JoyAim aim) const;
    void setType(const uint8_t type);
    bool isWithin(const int x1, const int y1, const int x2, const int y2) const;
    bool read(IFile &file);
    bool write(IFile &tfile) const;
    void reverveDir();
    const Pos pos() const override;
    int distance(const CActor &actor) const override;
    void move(const int16_t x, const int16_t y) override;
    void move(const Pos pos) override;
    inline int16_t getGranularFactor() const override { return ACTOR_GRANULAR_FACTOR; };

private:
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_type;
    JoyAim m_aim;
    uint8_t m_pu;
    template <typename ReadFunc>
    bool readCommon(ReadFunc readfile);
    template <typename WriteFunc>
    bool writeCommon(WriteFunc writefile) const;
    friend class CGame;
    friend class CBoss;
    enum : uint16_t
    {
        ACTOR_GRANULAR_FACTOR = 1
    };
};
