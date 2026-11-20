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
#include <functional>
#include <vector>
#include "sheetdata.h"

class CGame;

enum Sfx : uint16_t;
enum SfxTimeout : uint16_t;

// classic sfx_t
// frame size: 16px
// coordinates are calculed in 16px tiles
// uses: animz sequences
struct sfx_t
{
    int16_t x;
    int16_t y;
    Sfx sfxID;
    uint16_t timeout;

    bool isWithin(const int x1, const int y1, const int x2, const int y2) const
    {
        return (x >= x1) && (x < x2) && (y >= y1) && (y < y2);
    }
};

enum Sfx : uint16_t
{
    MIN_SFX = 0xf0,
    SFX_SPARKLE = MIN_SFX,
    SFX_EXPLOSION1, // fireball explosion
    SFX_EXPLOSION5, // barrel explosion
    SFX_EXPLOSION6, // icecube melting
    SFX_EXPLOSION7, // thunderbolt destruction (yellow)
    SFX_EXPLOSION0, // placeholder for mob
    SFX_FLAME,      // barrel flame
    MAX_SFX
};

enum SfxTimeout : uint16_t
{
    SFX_SPARKLE_TIMEOUT = 200,
    SFX_EXPLOSION1_TIMEOUT = 10,
    SFX_EXPLOSION6_TIMEOUT = 10,
    SFX_EXPLOSION7_TIMEOUT = 10,
    SFX_EXPLOSION0_TIMEOUT = 10,
    SFX_EXPLOSION5_TIMEOUT = 10,
    SFX_FLAME_TIMEOUT = 20,
};

// ---------------------------------------------------------------------------
// extended fx (spec docs/spec/gamespec/extended-fx.md)
// pixel coordinates, multi-frame, up to two layers, owned by CGame.
// frame sequences come from cs3-data-private/pixels/sfx.idx (imageID runs).
// ---------------------------------------------------------------------------

// new id space, independent of Sfx (MIN_SFX = 0xf0); fx never travels
// the sprite_t/tileID pipeline
enum Fx : uint16_t
{
    FX_BLUE_LIGHTNING = 0,
    FX_FIRE_COLUMN,
    FX_MUSHROOM_EXPLOSION,
    FX_ICE_SPIKES,
    FX_SMOKE_BOMB,
    FX_EXPLOSION, // two-layer: back 34..39 + front 40..45
    FX_BOMB_FUSE_LIT,   // dungeon_gathering: front 46..56
    FX_BOMB_EXPLOSION,  // dungeon_gathering: front 57..64
    MAX_FX
};

enum FxFlags : uint8_t
{
    FX_ONESHOT = 0x01,    // plays once, ends at frames * speed (spec I1)
    FX_REPEATABLE = 0x02, // loops (modulo), ends at ttl (spec I1)
};

// per-instance wire record: pure POD, raw-serialized, 12 bytes
struct fx_t
{
    int16_t x;        // world px, anchor = frame center
    int16_t y;
    int16_t ox;       // origin tile coords; -1 = not available (px spawn)
    int16_t oy;
    int16_t effectID; // index into g_fxData
    uint16_t timer;   // ticks alive, counts up from 0

    bool isWithin(const int x1, const int y1, const int x2, const int y2) const
    {
        return (x >= x1) && (x < x2) && (y >= y1) && (y < y2);
    }
};

// one layer's sequence on the fx sheet
struct fx_layer_t
{
    uint16_t base;  // sheet-relative first frameID
    uint16_t count; // 0 = layer unused
};

// reserved mechanics provision (spec 1.4): declared, no consumer yet
struct fx_hitbox_t
{
    int16_t dx;
    int16_t dy;
    uint16_t w;
    uint16_t h;
    int type; // mirrors BossData::HitBoxType
};

using FxEndFn = std::function<void(CGame &, const fx_t &)>;

// static definition table entry (g_fxData); never serialized
struct fxData_t
{
    uint16_t id;     // Fx enum value
    uint16_t sheet;  // fx sheet index (single sheet for now)
    uint8_t speed;   // ticks per frame (boss speed_anime style)
    uint8_t flags;   // FxFlags
    uint16_t ttl;    // lifetime (ticks) for FX_REPEATABLE
    uint16_t w, h;   // max frame extents, culling only
    fx_layer_t back; // drawn before actors
    fx_layer_t front;// drawn after actors
    int16_t damage = 0;               // reserved, unused
    uint8_t splashRadius = 0;         // reserved, unused
    const fx_hitbox_t *hitboxes = nullptr; // reserved, unused
    uint8_t hitboxCount = 0;          // reserved, unused
    FxEndFn onEnd; // fired by tickFx; per-effect, table-owned
};

// world-px view bounds for the culling query (spec S2)
struct fxView_t
{
    int left;
    int top;
    int right;
    int bottom;
};

// renderer-facing draw entry (never serialized); frameID is fx-sheet-relative
struct fxDraw_t
{
    int16_t x;
    int16_t y;
    uint16_t frameID;
};

extern std::vector<fxData_t> g_fxData;
// frame rects in imageID order; filled at load from the sfx.png obLT
// metadata chunk, not hardcoded
extern std::vector<tileinfo_t> g_fxSheet_data;

// bounds-checked table lookup; nullptr for unknown id
const fxData_t *fxDataAt(int effectID);

// derived frame index (spec I4): one-shot clamps at count-1, repeatable
// wraps modulo; returns layer.base + sequence index
uint16_t fxFrameIndex(const fx_t &fx, const fxData_t &def, const fx_layer_t &layer);

// decode the private obLT chunk payload carried inside sfx.png:
// u32 reserved, u32 version (2), u32 count, then count * {u16 x, y, w, h}
// little-endian; records appended in imageID order
bool parseFxSheetMeta(const uint8_t *data, size_t size, std::vector<tileinfo_t> &out);
