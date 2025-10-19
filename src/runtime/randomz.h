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

class Random
{
public:
    // Initialize with a fixed seed and optional tick
    explicit Random(uint32_t seed = 12345, uint32_t tick = 0);

    // Get next pseudo-random number (0 to UINT32_MAX)
    uint32_t next();

    // Get random number in range [min, max)
    int range(int min, int max);

    // Update with ticks from manageBosses
    void setTick(uint32_t tick);

    // Reset to initial seed and tick
    void reset();

private:
    uint32_t state_;
    uint32_t initialSeed_;
    uint32_t tick_;
};

// Precomputed random table for fast lookups
namespace PrecomputedRandom
{
    constexpr uint32_t TABLE_SIZE = 1024;
    extern const uint32_t table[TABLE_SIZE];

    // Get value using ticks
    uint32_t get(uint32_t tick);
}
