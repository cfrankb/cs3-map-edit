#pragma once

#include <cstdint>

struct layerdata_t {
    uint8_t nextTile;
    uint8_t animeSpeed;
    uint8_t tileType;
    uint8_t weight;
    const char *tag;
};

// Must stay const-qualified: src/runtime/*.cpp are compiled against
// src/runtime/layerdata.h, which declares 'extern const layerdata_t
// g_layerdata[256]'. MSVC encodes the const qualifier in the mangled name,
// so a non-const definition would not satisfy the runtime objects (LNK2001).
extern const layerdata_t g_layerdata[];

constexpr inline const layerdata_t & getLayerTileDef(int i) {
    return g_layerdata[i];
}
