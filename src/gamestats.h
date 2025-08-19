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
};

class CGameStats
{
public:
    CGameStats();
    ~CGameStats();

    int &get(const GameStat key);
    void set(const GameStat key, int value);
    void dec(const GameStat key);

    bool read(FILE *sfile);
    bool write(FILE *tfile);

private:
    std::unordered_map<uint16_t, int32_t> m_stats;
};