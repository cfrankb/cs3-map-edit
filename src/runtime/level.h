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
#include <vector>
#include <string>
class CMap;

typedef std::vector<std::string> StringVector;
std::vector<uint8_t> readFile(const char *fname);
bool processLevel(CMap &map, const char *fname);
bool convertCs3Level(CMap &map, const char *fname);
void splitString(const std::string str, StringVector &list);
bool getChMap(const char *mapFile, char *chMap);
bool fetchLevel(CMap &map, const char *fname, std::string &error);
