#pragma once
#include <vector>
#include <cstdint>

struct Stamp
{
    std::vector<uint8_t> tiles = {}; // for multi-tile stamps
    int cols = 0;
    int rows = 0;
    uint16_t baseID = MainTilesetBaseID;

    enum : uint16_t
    {
        MainTilesetBaseID = 0,    // tileset
        OtherTilesetBaseID = 256, // other tileset
    };
};
