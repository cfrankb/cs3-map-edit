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
#include <cstring>
#include <stdio.h>
#include "level.h"
#include "map.h"
#include "tilesdata.h"
#include "logger.h"
#include "shared/FileWrap.h"
#include "strhelper.h"
#include "shared/helper.h"

constexpr const int CS3_MAP_LEN = 64;
constexpr const int CS3_MAP_HEI = 64;
constexpr const int CS3_MAP_OFFSET = 7;

void splitString(const std::string str, StringVector &list)
{
    int i = 0;
    unsigned int j = 0;
    while (j < str.length())
    {
        if (isspace(str[j]))
        {
            list.emplace_back(str.substr(i, j - i));
            while (isspace(str[j]) && j < str.length())
            {
                ++j;
            }
            i = j;
            continue;
        }
        ++j;
    }
    list.emplace_back(str.substr(i, j - i));
}

bool getChMap(const char *mapFile, char *chMap)
{
    auto data = readFile(mapFile);
    if (data.empty())
    {
        LOGE("cannot read %s\n", mapFile);
        return false;
    }
    std::string p(reinterpret_cast<char *>(data.data()));
    LOGI("parsing tiles.map: %zu\n", p.length());
    int i = 0;
    size_t pos = 0;
    while (pos < p.length())
    {
        std::string current = processLine(p, pos);
        if (current.empty())
            continue;
        StringVector list;
        splitString2(current, list);
        if (list.size() < 4)
            continue;
        uint8_t ch = std::stoi(list[3], 0, 16);
        chMap[ch] = i;
        ++i;
    }
    return true;
}

bool processLevel(CMap &map, const char *fname)
{
    auto data = readFile(fname);
    if (data.empty())
    {
        LOGE("failed read: %s\n", fname);
        return false;
    }
    std::string input(reinterpret_cast<char *>(data.data()));
    size_t pos = 0;
    int maxRows = 0;
    int maxCols = 0;
    // Calculate dimensions
    while (pos < input.size())
    {
        std::string line = processLine(input, pos);
        if (line.empty())
            continue;
        maxCols = std::max(maxCols, static_cast<int>(line.size()));
        ++maxRows;
    }
    // Resize map
    map.clear();
    map.resize(maxCols, maxRows, '\0', true);
    // Set tiles
    pos = 0;
    int y = 0;
    while (pos < input.size())
    {
        std::string line = processLine(input, pos);
        if (!line.empty())
            for (int x = 0; x < static_cast<int>(line.size()) && x < maxCols; ++x)
                map.set(x, y, getChTile(line[x]));
        ++y;
    }
    return y > 0;
}

bool convertCs3Level(CMap &map, const char *fname)
{
    const uint16_t convTable[] = {
        TILES_BLANK,
        TILES_WALL_BRICK,
        TILES_ANNIE2,
        TILES_STOP,
        TILES_DIAMOND,
        TILES_AMULET1,
        TILES_CHEST,
        TILES_TRIFORCE,
        TILES_BOULDER,
        TILES_KEY01,
        TILES_DOOR01,
        TILES_KEY02,
        TILES_DOOR02,
        TILES_KEY03,
        TILES_DOOR03,
        TILES_KEY04,
        TILES_DOOR04,
        TILES_WALLS93,
        TILES_WALLS93_2,
        TILES_WALLS93_3,
        TILES_WALLS93_4,
        TILES_WALLS93_5,
        TILES_WALLS93_6,
        TILES_WALLS93_7,
        TILES_FLOWERS_2,
        TILES_TREE,
        TILES_ROCKS0,
        TILES_ROCKS1,
        TILES_ROCKS2,
        TILES_TOMB,
        TILES_SWAMP,
        TILES_VAMPLANT,
        TILES_INSECT1,
        TILES_TEDDY93,
        TILES_OCTOPUS,
        TILES_BLANK,
        TILES_BLANK,
        TILES_BLANK,
        TILES_DIAMOND + 0x100,
        TILES_WALLS93_2 + 0x100,
        TILES_DIAMOND + 0x200,
        TILES_WALLS93_2 + 0x200,
        TILES_DIAMOND + 0x300,
        TILES_WALLS93_2 + 0x300,
        TILES_DIAMOND + 0x400,
        TILES_BLANK + 0x400,
        TILES_DIAMOND + 0x500,
        TILES_BLANK + 0x500,
        TILES_DIAMOND + 0x600,
        TILES_BLANK + 0x600, // 0x31
        TILES_BLANK,         // 0x32
        TILES_BLANK,         // 0x33
        TILES_BLANK,         // 0x34
        TILES_YAHOO,         // 0x35
    };

    auto data = readFile(fname);
    if (data.size() == 0)
    {
        LOGE("failed read: %s\n", fname);
        return false;
    }

    map.clear();
    map.resize(CS3_MAP_LEN, CS3_MAP_HEI, '\0', true);
    auto p = CS3_MAP_OFFSET;
    for (int y = 0; y < CS3_MAP_LEN; ++y)
    {
        for (int x = 0; x < CS3_MAP_HEI; ++x)
        {
            uint8_t oldTile = data[p];
            if (oldTile >= sizeof(convTable) / 2)
            {
                LOGI("oldTile: %d\n", oldTile);
                oldTile = 0;
            }
            const uint16_t data = convTable[oldTile];
            const uint8_t attr = static_cast<uint8_t>(data >> 8);
            const uint8_t tile = static_cast<uint8_t>(data & 0xff);
            map.set(x, y, tile);
            map.setAttr(x, y, attr);
            ++p;
        }
    }
    return true;
}

bool fetchLevel(CMap &map, const char *fname, std::string &error)
{
    const int bufferSize = strlen(fname) + 128;
    std::vector<char> tmp(bufferSize);
    LOGI("fetching: %s\n", fname);

    FILE *sfile = fopen(fname, "rb");
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (!sfile)
    {
        snprintf(tmp.data(), bufferSize, "can't open file: %s", fname);
        error = tmp.data();
        return false;
    }

    const char MAPZ_SIGNATURE[] = {'M', 'A', 'P', 'Z'};
    char sig[sizeof(MAPZ_SIGNATURE)] = {0, 0, 0, 0};
    readfile(sig, sizeof(sig));
    if (memcmp(sig, MAPZ_SIGNATURE, sizeof(MAPZ_SIGNATURE)) == 0)
    {
        fclose(sfile);
        LOGI("level is MAPZ\n");
        return map.read(fname);
    }

    fseek(sfile, 0, SEEK_END);
    int size = ftell(sfile);
    if (size == CS3_MAP_LEN * CS3_MAP_HEI + CS3_MAP_OFFSET)
    {
        fclose(sfile);
        LOGI("level is cs3\n");
        return convertCs3Level(map, fname);
    }

    fclose(sfile);
    LOGI("level is map\n");
    return processLevel(map, fname);
}
