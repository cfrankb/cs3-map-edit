#ifndef DATA_H
#define DATA_H

#include <stdint.h>
#define TEXTURE_WIDTH 256
#define TEXTURE_HEIGHT 256
#define FONT_TEXTURE_WIDTH 128
#define FONT_TEXTURE_HEIGHT 64
#define FONT_SIZE 8
#define TEX_TILE_SIZE 16
#define TILE_SIZE 32

uint32_t * loadTileData();
uint32_t * loadFontData();

#endif // DATA_H
