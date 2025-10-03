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

#include <unordered_map>
#include <cstdint>

class IFile;
using colorMap_t = std::unordered_map<uint32_t, uint32_t>;

/// Stores color mappings for game effects.
struct ColorMaps
{
    colorMap_t sugarRush; ///< Color mappings for sugar rush mode.
    colorMap_t godMode;   ///< Color mappings for god mode.
    colorMap_t rage;      ///< Color mappings for rage mode.
};

/// Clears all color mappings.
void clearColorMaps(ColorMaps &colorMaps);
/// Parses color mappings from a file in format: [section] key=value.
bool parseColorMaps(IFile &file, ColorMaps &colorMaps);
/// Parses color mappings from a char buffer.
bool parseColorMaps(char *tmp, ColorMaps &colorMaps);