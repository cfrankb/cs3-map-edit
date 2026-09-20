#pragma once
#include <vector>
#include <cstdint>

struct Stamp
{
    std::vector<uint16_t> tiles = {}; // for multi-tile stamps
    int cols = 0;
    int rows = 0;
    uint16_t baseID = MainTilesetBaseID;

    enum : uint16_t
    {
        MainTilesetBaseID = 0x0,    // tileset
        OtherTilesetBaseID = 0x100, // other tileset
    };
};
