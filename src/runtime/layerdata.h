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

#include <cstdint>
#include <string>

struct layerdata_t
{
    uint8_t nextTile;
    uint8_t animeSpeed;
    uint8_t tileType;
    uint8_t weight;
    uint8_t granular;
    const char *tag;
};

const uint8_t GranualarUL = 1; // TOP LEFT
const uint8_t GranualarUR = 2; // TOP RIGHT
const uint8_t GranualarDL = 4; // BOTTOM LEFT
const uint8_t GranualarDR = 8; // BOTTOM RIGHT

enum LayerTileType
{
    Background,
    Foreground,
    Solid,
    Deadly,
    Water,
};

enum
{
    TOTAL_TILE_COUNT = 256
};
extern layerdata_t g_layerdata[TOTAL_TILE_COUNT];

bool loadTileLayer(const std::string &filename, layerdata_t *layers);

inline const layerdata_t &getLayerTileDef(int i)
{
    return g_layerdata[i];
}
