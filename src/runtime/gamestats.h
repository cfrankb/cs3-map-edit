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

#include <cstdint>
#include <cstdio>
#include <unordered_map>

class IFile;

enum GameStat : uint16_t
{
    S_GOD_MODE_TIMER = 1,
    S_EXTRA_SPEED_TIMER,
    S_SUGAR,
    S_SKILL,
    S_RAGE_TIMER,
    S_CLOSURE,
    S_CLOSURE_TIMER,
    S_REVEAL_EXIT,
    S_IDLE_TIME,
    S_FREEZE_TIMER,
    S_USER,
    S_TIME_TAKEN,
    S_PLAYER_HURT,
    S_CHUTE,
};

class CGameStats
{
public:
    CGameStats();
    ~CGameStats();

    int at(const GameStat key) const;
    int &get(const GameStat key);
    void set(const GameStat key, int value);
    int &dec(const GameStat key);
    int &inc(const GameStat key);

    [[deprecated("Use IFile interface instead")]]
    bool read(FILE *sfile);
    [[deprecated("Use IFile interface instead")]]
    bool write(FILE *tfile) const;

    bool read(IFile &sfile);
    bool write(IFile &tfile) const;

private:
    template <typename WriteFunc>
    bool writeCommon(WriteFunc writefile) const;
    template <typename ReadFunc>
    bool readCommon(ReadFunc readfile);
    std::unordered_map<uint16_t, int32_t> m_stats;
};