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
#include <memory>
#include "gamemixin.h"
#include "tilesdata.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "shared/FileWrap.h"
#include "map.h"
#include "game.h"
#include "maparch.h"
#include "animator.h"
#include "chars.h"
#include "recorder.h"
#include "events.h"
#include "states.h"
#include "statedata.h"
#include "sounds.h"
#include "sprtypes.h"
#include "gamestats.h"
#include "attr.h"
#include "logger.h"
#include "strhelper.h"
#include "filemacros.h"
#include "defaults.h"
#include "boss.h"
#include "gamesfx.h"
#include "tilesdefs.h"

// Check windows
#ifdef _WIN64
#define ENVIRONMENT64
#else
#ifdef _WIN32
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

CGameMixin::CGameMixin()
{
    m_game = CGame::getGame();
    m_animator = std::make_unique<CAnimator>();
    m_prompt = PROMPT_NONE;
    clearJoyStates();
    clearScores();
    clearKeyStates();
    clearButtonStates();
    m_recorder = std::make_unique<CRecorder>();
    m_eventCountdown = 0;
    m_currentEvent = EVENT_NONE;
    initUI();
    _WIDTH = DEFAULT_WIDTH;
    _HEIGHT = DEFAULT_HEIGHT;
}

CGameMixin::~CGameMixin()
{
}

/**
 * @brief Draw String at screen coordonates (x,y)
 *
 * @param frame targe pixmap buffer
 * @param x x-pos
 * @param y y-pos
 * @param text null terminated string
 * @param color text color
 * @param bgcolor background color. use Clear for transparent
 * @param scaleX x-scalling factor for font
 * @param scaleY y-scalling factor for font
 */
void CGameMixin::drawFont(CFrame &frame, int x, int y, const char *text, const Color color, const Color bgcolor, const int scaleX, const int scaleY)
{
    uint32_t *rgba = frame.getRGB().data();
    const int rowPixels = frame.width();
    const int fontSize = FONT_SIZE;
    const int fontOffset = fontSize;
    const int textSize = strlen(text);
    for (int i = 0; i < textSize; ++i)
    {
        uint8_t c = static_cast<uint8_t>(text[i]);
        const uint8_t *font = c >= CHARS_CUSTOM ? getCustomChars() + (c - CHARS_CUSTOM) * fontOffset
                                                : m_fontData.data() + (c - ' ') * fontOffset;
        for (int yy = 0; yy < fontSize; ++yy)
        {
            for (int xx = 0; xx < fontSize; ++xx)
            {
                const uint8_t bitFilter = 1 << xx;
                const uint32_t &pixColor = font[yy] & bitFilter ? color : bgcolor;
                for (int stepY = 0; stepY < scaleY; ++stepY)
                {
                    for (int stepX = 0; stepX < scaleX; ++stepX)
                    {
                        if (pixColor)
                            rgba[(yy * scaleY + y + stepY) * rowPixels + xx * scaleX + x + stepX] = pixColor;
                    }
                }
            }
        }
        x += fontSize * scaleX;
    }
}

void CGameMixin::drawFont6x6(CFrame &bitmap, int x, int y, const char *text, const Color color, const Color bgcolor)
{
    constexpr int fontSize = 6;
    constexpr int fontOffset = FONT_SIZE;
    const int textSize = strlen(text);
    constexpr int rowIndex[] = {0, 2, 3, 4, 5, 6};
    constexpr int colIndex[] = {0, 2, 3, 4, 5, 6};
    enum
    {
        SKIP,
        CONTINUE,
        STOP
    };

    auto boundCheck = [&bitmap](int rx, int ry)
    {
        if (rx >= bitmap.width() || ry >= bitmap.height())
            return STOP;
        else if (rx < 0 || ry < 0)
            return SKIP;
        else
            return CONTINUE;
    };

    for (int i = 0; i < textSize; ++i)
    {
        const uint8_t c = static_cast<uint8_t>(text[i]);
        const uint8_t *font = c >= CHARS_CUSTOM ? getCustomChars() + (c - CHARS_CUSTOM) * fontOffset
                                                : m_fontData.data() + (c - ' ') * fontOffset;

        for (int row = 0; row < fontSize; ++row)
        {
            const uint8_t &fbits = font[rowIndex[row]];
            const int rx = x + i * fontSize;
            const int ry = y + row;
            const int result = boundCheck(rx, ry);
            if (result == STOP)
                break;
            else if (result == SKIP)
                continue;

            uint32_t *dest = &bitmap.at(rx, ry);
            for (int col = 0; col < fontSize; ++col)
            {
                const int result = boundCheck(rx + col, ry);
                if (result == STOP)
                    break;
                else if (result == SKIP)
                    continue;

                const uint8_t bitmask = 1 << colIndex[col];
                if (fbits & bitmask)
                {
                    dest[col] = color;
                }
                else if (bgcolor != CLEAR)
                {
                    dest[col] = bgcolor;
                }
            }
        }
    }
}

/**
 * @brief draw Rectangle into pixmap buffer
 *
 * @param frame target pixmap
 * @param rect rectangle
 * @param color color
 * @param fill fill ?
 */
void CGameMixin::drawRect(CFrame &frame, const rect_t &rect, const Color color, bool fill)
{
    uint32_t *rgba = frame.getRGB().data();
    const int rowPixels = frame.width();
    if (fill)
    {
        for (int y = 0; y < rect.height; y++)
        {
            for (int x = 0; x < rect.width; x++)
            {
                rgba[(rect.y + y) * rowPixels + rect.x + x] = color;
            }
        }
    }
    else
    {
        for (int y = 0; y < rect.height; y++)
        {
            for (int x = 0; x < rect.width; x++)
            {
                if (y == 0 || y == rect.height - 1 || x == 0 || x == rect.width - 1)
                {
                    rgba[(rect.y + y) * rowPixels + rect.x + x] = color;
                }
            }
        }
    }
}

/**
 * @brief draw a tile into a pixmap buffer. this a special variant that can draw partial tiles and perform dynamic recoloring
 *
 * @param bitmap
 * @param x
 * @param y
 * @param tile
 * @param rect
 * @param colorMask
 * @param colorMap
 */

void CGameMixin::drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const rect_t &rect, const ColorMask colorMask, std::unordered_map<uint32_t, uint32_t> *colorMap)
{
    const int width = bitmap.width();
    uint32_t *dest = bitmap.getRGB().data() + x + y * width;
    if (!colorMask && !colorMap)
    {
        for (int row = 0; row < rect.height; ++row)
        {
            for (int col = 0; col < rect.width; ++col)
            {
                auto color = tile.at(col + rect.x, row + rect.y);
                if (!(color & ALPHA))
                    continue;
                dest[col] = color;
            }
            dest += width;
        }
    }
    else
    {
        const uint32_t colorFilter = fazFilter(FAZ_INV_BITSHIFT);
        for (int row = 0; row < rect.height; ++row)
        {
            for (int col = 0; col < rect.width; ++col)
            {
                uint32_t color = tile.at(col + rect.x, row + rect.y);
                if (!(color & ALPHA))
                    continue;
                if (colorMap && colorMap->count(color))
                {
                    color = (*colorMap)[color];
                }
                if (colorMask == COLOR_FADE)
                {
                    color = ((color >> FAZ_INV_BITSHIFT) & colorFilter) | ALPHA;
                }
                else if (colorMask == COLOR_INVERTED)
                {
                    color ^= 0x00ffffff;
                }
                else if (colorMask == COLOR_GRAYSCALE)
                {
                    uint8_t *c = reinterpret_cast<uint8_t *>(&color);
                    const uint16_t avg = (c[0] + c[1] + c[2]) / 3;
                    c[0] = c[1] = c[2] = avg;
                }
                else if (colorMask == COLOR_ALL_WHITE)
                {
                    color = WHITE;
                }
                dest[col] = color;
            }
            dest += width;
        }
    }
}

/**
 * @brief draw a tile into pixmap buffer. this version is optimized for speed and can only draw complete tiles
 *
 * @param bitmap
 * @param x
 * @param y
 * @param tile
 * @param alpha
 * @param colorMask
 * @param colorMap
 */

void CGameMixin::drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const bool alpha, const ColorMask colorMask, std::unordered_map<uint32_t, uint32_t> *colorMap)
{
    const int width = bitmap.width();
    const uint32_t *tileData = tile.getRGB().data();
    uint32_t *dest = bitmap.getRGB().data() + x + y * width;
    if (alpha || colorMask || colorMap)
    {
        const uint32_t colorFilter = fazFilter(FAZ_INV_BITSHIFT);
        for (uint32_t row = 0; row < TILE_SIZE; ++row)
        {
            for (uint32_t col = 0; col < TILE_SIZE; ++col)
            {
                uint32_t color = tileData[col];
                if (!color)
                    continue;
                if (colorMap && colorMap->count(color))
                {
                    color = (*colorMap)[color];
                }
                if (colorMask == COLOR_FADE)
                {
                    color = ((color >> FAZ_INV_BITSHIFT) & colorFilter) | ALPHA;
                }
                else if (colorMask == COLOR_INVERTED)
                {
                    color ^= 0x00ffffff;
                }
                else if (colorMask == COLOR_GRAYSCALE)
                {
                    uint8_t *c = reinterpret_cast<uint8_t *>(&color);
                    const uint16_t avg = (c[0] + c[1] + c[2]) / 3;
                    c[0] = c[1] = c[2] = avg;
                }
                else if (colorMask == COLOR_ALL_WHITE)
                {
                    color = WHITE;
                }
                dest[col] = color;
            }
            dest += width;
            tileData += TILE_SIZE;
        }
    }
    else
    {

#ifdef ENVIRONMENT64
        uint64_t *d64 = reinterpret_cast<uint64_t *>(dest);
        const uint64_t *s64 = reinterpret_cast<const uint64_t *>(tileData);
        for (uint32_t row = 0; row < TILE_SIZE; ++row)
        {
            int i = 0;
            d64[i++] = *(s64++);
            d64[i++] = *(s64++);
            d64[i++] = *(s64++);
            d64[i++] = *(s64++);

            d64[i++] = *(s64++);
            d64[i++] = *(s64++);
            d64[i++] = *(s64++);
            d64[i++] = *(s64++);
            d64 += width / 2;
        }
#else
        for (uint32_t row = 0; row < TILE_SIZE; ++row)
        {
            int i = 0;

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);

            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest[i++] = *(tileData++);
            dest += width;
        }
#endif
    }
}

void CGameMixin::drawTileFaz(CFrame &bitmap, const int x, const int y, CFrame &tile, int fazBitShift, const ColorMask colorMask)
{
    const int width = bitmap.width();
    const uint32_t *tileData = tile.getRGB().data();
    uint32_t *dest = bitmap.getRGB().data() + x + y * width;
    const uint32_t colorFilter = fazBitShift ? fazFilter(fazBitShift) : 0;
    for (uint32_t row = 0; row < TILE_SIZE; ++row)
    {
        for (uint32_t col = 0; col < TILE_SIZE; ++col)
        {
            uint32_t color = tileData[col];
            if (!color)
                continue;
            if (fazBitShift)
                color = ((color >> fazBitShift) & colorFilter) | ALPHA;
            if (colorMask == COLOR_INVERTED)
            {
                color ^= 0x00ffffff;
            }
            else if (colorMask == COLOR_GRAYSCALE)
            {
                uint8_t *c = reinterpret_cast<uint8_t *>(&color);
                const uint16_t avg = (c[0] + c[1] + c[2]) / 3;
                c[0] = c[1] = c[2] = avg;
            }
            else if (colorMask == COLOR_ALL_WHITE)
            {
                color = WHITE;
            }
            dest[col] = color;
        }
        dest += width;
        tileData += TILE_SIZE;
    }
}

void CGameMixin::drawKeys(CFrame &bitmap)
{
    CGame &game = *m_game;
    CFrameSet &tiles = *m_tiles;
    const int y = getHeight() - TILE_SIZE;
    int x = getWidth() - TILE_SIZE;
    const CGame::userKeys_t &keys = game.keys();
    for (size_t i = 0; i < CGame::MAX_KEYS; ++i)
    {
        const uint8_t &k = keys.tiles[i];
        const uint8_t &u = keys.indicators[i];
        if (k)
        {
            // add visual sfx for key pickup
            if (u == CGame::MAX_KEY_STATE)
                drawTileFaz(bitmap, x, y, *tiles[k], 0, COLOR_ALL_WHITE);
            else
                drawTileFaz(bitmap, x, y, *tiles[k], u);
            x -= TILE_SIZE;
        }
    }
    if ((m_ticks >> 1) & 1)
        game.decKeyIndicators();
}

void CGameMixin::drawScreen(CFrame &bitmap)
{
    const CGame &game = *m_game;

    // draw viewport
    if (m_cameraMode == CAMERA_MODE_DYNAMIC)
        drawViewPortDynamic(bitmap);
    else if (m_cameraMode == CAMERA_MODE_STATIC)
        drawViewPortStatic(bitmap);

    const int flash = game.statsConst().at(S_FLASH);
    if (flash)
        flashScreen(bitmap);

    const int hurtStage = game.statsConst().at(S_PLAYER_HURT);
    const bool isPlayerHurt = hurtStage != CGame::HurtNone;
    if (isPlayerHurt)
        for (int i = 0; i < SCREEN_SHAKES; ++i)
            bitmap.shiftLEFT(false);

    // visual cues
    const visualCues_t visualcues{
        .diamondShimmer = game.goalCount() < m_visualStates.rGoalCount,
        .livesShimmer = game.lives() > m_visualStates.rLives,
    };
    m_visualStates.rGoalCount = game.goalCount();
    m_visualStates.rLives = game.lives();

    // draw game status
    drawGameStatus(bitmap, visualcues);

    // draw bottom rect
    const bool isFullWidth = getWidth() >= MIN_WIDTH_FULL;

    if (m_currentEvent >= MSG0)
    {
        drawScroll(bitmap);
    }
    else if (m_currentEvent == EVENT_SUGAR)
    {
        const Color rectBG = isFullWidth && m_currentEvent >= MSG0 ? WHITE : DARKGRAY;
        const Color rectBorder = isPlayerHurt              ? PINK
                                 : visualcues.livesShimmer ? GREEN
                                                           : LIGHTGRAY;
        drawRect(bitmap, rect_t{0, bitmap.height() - 16, getWidth(), TILE_SIZE}, rectBG, true);
        drawRect(bitmap, rect_t{0, bitmap.height() - 16, getWidth(), TILE_SIZE}, rectBorder, false);
    }

    // draw current event text
    drawEventText(bitmap);

    if (!isFullWidth || m_currentEvent < MSG0)
    {
        // draw Healthbar
        drawHealthBar(bitmap, isPlayerHurt);

        // draw keys
        drawKeys(bitmap);
    }

    // drawButtons
    if (m_ui.isVisible())
        drawUI(bitmap, m_ui);

    // draw timeout
    drawTimeout(bitmap);
}

void CGameMixin::drawScroll(CFrame &bitmap)
{
    const CFrameSet &sheet = *m_uisheet;
    constexpr int SCROLL_LEFT = 0;
    constexpr int SCROLL_MID = 1;
    constexpr int SCROLL_RIGHT = 2;
    constexpr int scrollHeight = 48;
    constexpr int partWidth = 16;
    const int y = bitmap.height() - scrollHeight;
    constexpr rect_t rect{0, 0, partWidth, scrollHeight};
    drawTile(bitmap, 0, y, *sheet[SCROLL_LEFT], rect);
    for (int x = partWidth; x < bitmap.width() - partWidth; x += partWidth)
        drawTile(bitmap, x, y, *sheet[SCROLL_MID], rect);
    drawTile(bitmap, bitmap.width() - partWidth, y, *sheet[SCROLL_RIGHT], rect);
}

void CGameMixin::drawTimeout(CFrame &bitmap)
{
    const CMap *map = &m_game->getMap();
    const CStates &states = map->statesConst();
    const uint16_t timeout = states.getU(TIMEOUT);
    if (timeout)
    {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%.2d", timeout - 1);
        const bool lowTime = timeout <= 15;
        const int scaleX = !lowTime ? 3 : 5;
        const int scaleY = !lowTime ? 4 : 5;
        const int x = getWidth() - scaleX * FONT_SIZE * strlen(tmp) - FONT_SIZE;
        const int y = 2 * FONT_SIZE;
        const Color color = lowTime && (m_ticks >> 3) & 1 ? ORANGE : YELLOW;
        drawFont(bitmap, x, y, tmp, color, CLEAR, scaleX, scaleY);
    }
}

CFrame *CGameMixin::tile2Frame(const uint8_t tileID, ColorMask &colorMask, std::unordered_map<uint32_t, uint32_t> *&colorMap)
{
    const CGame &game = *m_game;
    CFrame *tile;
    if (tileID == TILES_STOP || tileID == TILES_BLANK || m_animator->isSpecialCase(tileID))
    {
        // skip blank tiles and special cases
        tile = nullptr;
    }
    else if (tileID == TILES_ANNIE2)
    {
        const uint8_t userID = game.getUserID();
        const uint32_t userBaseFrame = PLAYER_TOTAL_FRAMES * userID;
        const int aim = game.playerConst().getAim();
        const CFrameSet &annie = *m_users;
        if (!game.health())
        {
            tile = annie[INDEX_PLAYER_DEAD * PLAYER_FRAMES + m_playerFrameOffset + userBaseFrame];
        }
        else if (!game.goalCount() && game.isClosure())
        {
            tile = annie[static_cast<uint8_t>(AIM_DOWN) * PLAYER_FRAMES + m_playerFrameOffset + userBaseFrame];
        }
        else if (aim == AIM_DOWN && game.m_gameStats->get(S_IDLE_TIME) > IDLE_ACTIVATION)
        {
            const int idleTime = game.m_gameStats->get(S_IDLE_TIME);
            const int idleFrame = PLAYER_IDLE_BASE + ((idleTime >> 4) & 3);
            const int frame = idleTime & 0x08 ? idleFrame : static_cast<int>(PLAYER_DOWN_INDEX);
            tile = annie[frame + userBaseFrame];
        }
        else if (game.m_gameStats->get(S_BOAT) != 0 && game.playerConst().getPU() == TILES_SWAMP)
        {
            tile = annie[PLAYER_BOAT_FRAME + userBaseFrame];
        }
        else
        {
            tile = annie[aim * PLAYER_FRAMES + m_playerFrameOffset + userBaseFrame];
        }

        const int hurtStage = game.statsConst().at(S_PLAYER_HURT);
        if (hurtStage == CGame::HurtFlash)
            colorMask = COLOR_ALL_WHITE;
        else if (hurtStage == CGame::HurtInv)
            colorMask = COLOR_INVERTED;
        else if (hurtStage == CGame::HurtFaz)
            colorMask = COLOR_FADE;
        else
            colorMask = COLOR_NOCHANGE;

        if (m_game->isFrozen())
        {
            colorMask = COLOR_GRAYSCALE;
        }
        else if (m_game->hasExtraSpeed())
        {
            colorMap = &m_colormaps.sugarRush;
        }
        else if (m_game->isGodMode())
        {
            colorMap = &m_colormaps.godMode;
        }
        else if (m_game->isRageMode())
        {
            colorMap = &m_colormaps.rage;
        }
    }
    else
    {
        const uint16_t j = m_animator->at(tileID);
        if (j == NO_ANIMZ)
        {
            const CFrameSet &tiles = *m_tiles;
            tile = tiles[tileID];
        }
        else
        {
            const CFrameSet &animz = *m_animz;
            tile = animz[j];
        }
    }
    return tile;
}

void CGameMixin::gatherSprites(std::vector<sprite_t> &sprites, const cameraContext_t &context)
{
    CGame &game = *m_game;
    CMap *map = &m_game->getMap();
    const int maxRows = getHeight() / TILE_SIZE;
    const int maxCols = getWidth() / TILE_SIZE;
    const int rows = std::min(maxRows, map->height());
    const int cols = std::min(maxCols, map->width());
    const int &mx = context.mx;
    const int &ox = context.ox;
    const int &my = context.my;
    const int &oy = context.oy;
    const std::vector<CActor> &monsters = game.getMonsters();
    for (const auto &monster : monsters)
    {
        const uint8_t &tileID = map->at(monster.x(), monster.y());
        if (monster.isWithin(mx, my, mx + cols + ox, my + rows + oy) &&
            m_animator->isSpecialCase(tileID))
        {
            const Pos pos = monster.pos();
            const uint8_t attr = map->getAttr(pos.x, pos.y);
            sprites.emplace_back(
                sprite_t{.x = monster.x(),
                         .y = monster.y(),
                         .tileID = tileID,
                         .aim = monster.getAim(),
                         .attr = attr});
        }
    }

    const std::vector<sfx_t> &sfxAll = m_game->getSfx();
    for (const auto &sfx : sfxAll)
    {
        if (sfx.isWithin(mx, my, mx + cols + ox, my + rows + oy))
        {
            sprites.emplace_back(sprite_t{
                .x = sfx.x,
                .y = sfx.y,
                .tileID = sfx.sfxID,
                .aim = AIM_NONE,
                .attr = 0,
            });
        }
    }
}

void CGameMixin::drawViewPortDynamic(CFrame &bitmap)
{
    const CMap *map = &m_game->getMap();
    const int maxRows = getHeight() / TILE_SIZE;
    const int maxCols = getWidth() / TILE_SIZE;
    const int rows = std::min(maxRows, map->height());
    const int cols = std::min(maxCols, map->width());
    const int mx = m_cx / 2;
    const int ox = m_cx & 1;
    const int my = m_cy / 2;
    const int oy = m_cy & 1;
    const int halfOffset = TILE_SIZE / 2;
    const int tileSize = TILE_SIZE;

    bitmap.fill(BLACK);
    int py = oy ? -halfOffset : 0;
    for (int y = 0; y < rows + oy; ++y)
    {
        bool firstY = oy && y == 0;
        bool lastY = oy && y == rows;
        int px = ox ? -halfOffset : 0;
        for (int x = 0; x < cols + ox; ++x)
        {
            bool firstX = ox && x == 0;
            bool lastX = ox && x == cols;
            uint8_t tileID = map->at(x + mx, y + my);
            ColorMask colorMask = COLOR_NOCHANGE;
            std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr;
            CFrame *tile = tile2Frame(tileID, colorMask, colorMap);
            if (tile)
            {
                if (firstX || firstY || lastX || lastY)
                {
                    rect_t rect{
                        .x = !firstX ? 0 : halfOffset,
                        .y = !firstY ? 0 : halfOffset,
                        .width = !(firstX || lastX) ? tileSize : halfOffset,
                        .height = !(firstY || lastY) ? tileSize : halfOffset,
                    };
                    drawTile(bitmap,
                             !firstX ? px : 0,
                             !firstY ? py : 0,
                             *tile, rect, colorMask, colorMap);
                }
                else
                {
                    drawTile(bitmap, px, py, *tile, false, colorMask, colorMap);
                }
            }
            px += TILE_SIZE;
        }
        py += TILE_SIZE;
    }

    /////////////////////////////////////////////////////////////////////////////
    // overlay special case monsters and sfx

    std::vector<sprite_t> sprites;
    gatherSprites(sprites, {.mx = mx, .ox = ox, .my = my, .oy = oy});
    for (const auto &sprite : sprites)
    {
        // special case animations
        const int x = sprite.x - mx;
        const int y = sprite.y - my;
        CFrame *tile = calcSpecialFrame(sprite);
        bool firstY = oy && y == 0;
        bool lastY = oy && y == rows;
        bool firstX = ox && x == 0;
        bool lastX = ox && x == cols;
        int px = x * TILE_SIZE;
        int py = y * TILE_SIZE;
        if (x && ox)
            px -= halfOffset;
        if (y && oy)
            py -= halfOffset;

        if (firstX || firstY || lastX || lastY)
        {
            rect_t rect{
                .x = !firstX ? 0 : halfOffset,
                .y = !firstY ? 0 : halfOffset,
                .width = !(firstX || lastX) ? tileSize : halfOffset,
                .height = !(firstY || lastY) ? tileSize : halfOffset,
            };
            drawTile(bitmap, px, py, *tile, rect);
        }
        else
        {
            drawTile(bitmap, px, py, *tile, true);
        }
    }

    // draw Bosses
    drawBossses(bitmap, m_cx, m_cy, maxCols * CBoss::BOSS_GRANULAR_FACTOR, maxRows * CBoss::BOSS_GRANULAR_FACTOR);
}

void CGameMixin::drawViewPortStatic(CFrame &bitmap)
{
    const CMap *map = &m_game->getMap();
    const CGame &game = *m_game;

    const int maxRows = getHeight() / TILE_SIZE;
    const int maxCols = getWidth() / TILE_SIZE;
    const int rows = std::min(maxRows, map->height());
    const int cols = std::min(maxCols, map->width());

    const int lmx = std::max(0, game.playerConst().x() - cols / 2);
    const int lmy = std::max(0, game.playerConst().y() - rows / 2);
    const int mx = std::min(lmx, map->width() > cols ? map->width() - cols : 0);
    const int my = std::min(lmy, map->height() > rows ? map->height() - rows : 0);
    bitmap.fill(BLACK);
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            uint8_t tileID = map->at(x + mx, y + my);
            ColorMask inverted = COLOR_NOCHANGE;
            std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr;
            CFrame *tile = tile2Frame(tileID, inverted, colorMap);
            if (tile)
            {
                if (colorMap != nullptr || inverted)
                {
                    drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false, inverted, colorMap);
                }
                else
                {
                    drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);
                }
            }
        }
    }

    std::vector<sprite_t> sprites;
    gatherSprites(sprites, {.mx = mx, .ox = 0, .my = my, .oy = 0});
    for (const auto &sprite : sprites)
    {
        // special case animations
        const int x = sprite.x - mx;
        const int y = sprite.y - my;
        CFrame *tile = calcSpecialFrame(sprite);
        drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, true);
    }

    // draw Bosses
    drawBossses(bitmap,
                mx * CBoss::BOSS_GRANULAR_FACTOR,
                my * CBoss::BOSS_GRANULAR_FACTOR,
                maxCols * CBoss::BOSS_GRANULAR_FACTOR,
                maxRows * CBoss::BOSS_GRANULAR_FACTOR);
}

void CGameMixin::drawBossses(CFrame &bitmap, const int mx, const int my, const int sx, const int sy)
{
    auto between = [](int a1, int a2, int b1, int b2)
    {
        return a1 < b2 && a2 > b1;
    };

    auto betweenRect = [&between](const rect_t &bRect, const rect_t &sRect)
    {
        return between(bRect.x, bRect.x + bRect.width, sRect.x, sRect.x + sRect.width) &&
               between(bRect.y, bRect.y + bRect.height, sRect.y, sRect.y + sRect.height);
    };

    auto calcSize = [](const auto _x, const auto _wu, const auto _sx)
    {
        if (_x < 0)
            return _wu + _x;
        else if (_x + _wu > _sx)
            return _sx - _x;
        return _wu;
    };

    auto printRect = [](const rect_t &rect, const std::string_view &name)
    {
        LOGI("%s (%d, %d) w:%d h: %d", name.data(), rect.x, rect.y, rect.width, rect.height);
    };

    (void)printRect;

    constexpr int MAX_HP_GAUGE = 64;
    constexpr int GRID_SIZE = 8;
    constexpr int HP_BAR_HEIGHT = 8;
    constexpr int HP_BAR_SPACING = 2;

    // bosses are drawn on a 8x8 grid overlayed on top of the regular 16x16 grid
    for (const auto &boss : m_game->bosses())
    {
        // don't process completed bosses
        if (boss.isDone())
            continue;

        const std::vector<CFrame *> *frames = nullptr;
        if (boss.data()->sheet == 0)
        {
            frames = &(m_sheet0.get())->frames();
        }
        else if (boss.data()->sheet == 1)
        {
            frames = &(m_sheet1.get())->frames();
        }
        else
        {
            LOGE("invalid sheet: %d", boss.data()->sheet);
            continue;
        }

        const int num = boss.currentFrame();
        CFrame &frame = *(*frames)[num];
        const hitbox_t &hitbox = boss.hitbox();

        // Logical coordonates comverted to screen positions
        // (using GRID_SIZE)

        // Boss Rect
        const rect_t bRect{
            GRID_SIZE * (boss.x() - hitbox.x),
            GRID_SIZE * (boss.y() - hitbox.y),
            frame.width(),
            frame.height(),
        };

        // Screen Rect
        const rect_t sRect{
            .x = GRID_SIZE * mx,
            .y = GRID_SIZE * my,
            .width = GRID_SIZE * sx,
            .height = GRID_SIZE * sy,
        };

        if (betweenRect(bRect, sRect))
        {
            const int x = bRect.x - sRect.x;
            const int y = bRect.y - sRect.y;
            const rect_t rect{
                .x = x < 0 ? std::abs(x) : 0,
                .y = y < 0 ? std::abs(y) : 0,
                .width = calcSize(x, bRect.width, sRect.width),
                .height = calcSize(y, bRect.height, sRect.height),
            };
            // draw boss
            drawTile(bitmap,
                     x > 0 ? x : 0,
                     y > 0 ? y : 0,
                     frame,
                     rect);
        }

        // skip drawing the healthbar and name
        if (!boss.data()->show_details)
            continue;

        // Hp Rect
        const float hpRatio = (float)boss.maxHp() / MAX_HP_GAUGE;
        const rect_t hRect{
            .x = bRect.x,
            .y = bRect.y - HP_BAR_HEIGHT - HP_BAR_SPACING,
            .width = MAX_HP_GAUGE, // boss.maxHp(),
            .height = HP_BAR_HEIGHT,
        };
        if (betweenRect(hRect, sRect))
        {
            const int x = hRect.x - sRect.x;
            const int y = hRect.y - sRect.y;
            const rect_t rectFullHp{
                .x = std::max(0, x),
                .y = std::max(0, y),
                .width = calcSize(x, hRect.width, sRect.width),
                .height = calcSize(y, hRect.height, sRect.height),
            };
            const rect_t rectHp{
                .x = std::max(0, x),
                .y = std::max(0, y),
                .width = calcSize(x, static_cast<int>(boss.hp() / hpRatio), sRect.width),
                .height = calcSize(y, hRect.height, sRect.height),
            };
            // draw healthbar
            drawRect(bitmap, rectFullHp, BLACK, true);             // black background
            drawRect(bitmap, rectHp, boss.data()->color_hp, true); // orange hp bar
            drawRect(bitmap, rectFullHp, WHITE, false);            // white outline
        }

        // draw Boss Name
        const int x = hRect.x - sRect.x;
        const int y = hRect.y - sRect.y - FONT_SIZE;
        drawFont6x6(bitmap, x, y, boss.name(), boss.data()->color_name, CLEAR);
    }
}

void CGameMixin::centerCamera()
{
    const CMap *map = &m_game->getMap();
    const CGame &game = *m_game;
    const int maxRows = getHeight() / TILE_SIZE;
    const int maxCols = getWidth() / TILE_SIZE;
    const int rows = std::min(maxRows, map->height());
    const int cols = std::min(maxCols, map->width());
    const int lmx = std::max(0, game.playerConst().x() - cols / 2);
    const int lmy = std::max(0, game.playerConst().y() - rows / 2);
    const int mx = std::min(lmx, map->width() > cols ? map->width() - cols : 0);
    const int my = std::min(lmy, map->height() > rows ? map->height() - rows : 0);
    m_cx = mx * 2;
    m_cy = my * 2;
}

void CGameMixin::drawLevelIntro(CFrame &bitmap)
{
    const int mode = m_game->mode();
    char t[32];
    switch (mode)
    {
    case CGame::MODE_LEVEL_INTRO:
        snprintf(t, sizeof(t), "LEVEL %.2d", m_game->level() + 1);
        break;
    case CGame::MODE_RESTART:
        if (m_game->lives() > 1)
        {
            snprintf(t, sizeof(t), "LIVES LEFT %.2d", m_game->lives());
        }
        else
        {
            strncpy(t, "LAST LIFE !", sizeof(t));
        }
        break;
    case CGame::MODE_GAMEOVER:
        strncpy(t, "GAME OVER", sizeof(t));
        break;
    case CGame::MODE_TIMEOUT:
        strncpy(t, "OUT OF TIME", sizeof(t));
        break;
    case CGame::MODE_CHUTE:
        snprintf(t, sizeof(t), "DROP TO LEVEL %.2d", m_game->level() + 1);
    };

    const int x = (getWidth() - strlen(t) * 2 * FONT_SIZE) / 2;
    const int y = (getHeight() - FONT_SIZE * 2) / 2;
    bitmap.fill(BLACK);
    drawFont(bitmap, x, y, t, YELLOW, BLACK, 2, 2);

    if (mode == CGame::MODE_LEVEL_INTRO || mode == CGame::MODE_CHUTE)
    {
        const char *t = m_game->getMap().title();
        const int x = (getWidth() - strlen(t) * FONT_SIZE) / 2;
        drawFont(bitmap, x, y + 3 * FONT_SIZE, t, WHITE);
    }

    if (mode != CGame::MODE_GAMEOVER && getWidth() >= MIN_WIDTH_FULL)
    {
        const char *hint = m_game->getHintText();
        const int x = (getWidth() - strlen(hint) * FONT_SIZE) / 2;
        const int y = getHeight() - FONT_SIZE * 4;
        drawFont(bitmap, x, y, hint, CYAN);
    }
    m_currentEvent = EVENT_NONE;
    m_timer = TICK_RATE;
}

void CGameMixin::mainLoop()
{
    handleFunctionKeys();
    ++m_ticks;
    CGame &game = *m_game;
    if (game.mode() != CGame::MODE_CLICKSTART &&
        game.mode() != CGame::MODE_TITLE &&
        m_countdown > 0)
    {
        --m_countdown;
    }

    switch (game.mode())
    {
    case CGame::MODE_HISCORES:
        if (m_recordScore && inputPlayerName())
        {
            m_recordScore = false;
            saveScores();
        }
        [[fallthrough]];
    case CGame::MODE_LEVEL_INTRO:
        [[fallthrough]];
    case CGame::MODE_RESTART:
        [[fallthrough]];
    case CGame::MODE_GAMEOVER:
        [[fallthrough]];
    case CGame::MODE_CHUTE:
        [[fallthrough]];
    case CGame::MODE_TIMEOUT:
        if (m_countdown)
        {
            return;
        }
        if (game.mode() == CGame::MODE_GAMEOVER)
        {
            if (!m_hiscoreEnabled)
            {
                restartGame();
                return;
            }
            if (!m_scoresLoaded)
            {
                m_scoresLoaded = loadScores();
            }
            m_scoreRank = rankUserScore();
            m_recordScore = m_scoreRank != INVALID;
            m_countdown = HISCORE_DELAY;
            game.setMode(CGame::MODE_HISCORES);
#if defined(__ANDROID__)
            if (m_recordScore)
            {
                game.setMode(CGame::MODE_NEW_INPUTNAME);
            }
#endif
            return;
        }
        else if (game.mode() == CGame::MODE_HISCORES)
        {
            if (!m_recordScore)
            {
                restartGame();
            }
            return;
        }
        else
        {
            game.setMode(CGame::MODE_PLAY);
        }
        break;
    case CGame::MODE_IDLE:
    case CGame::MODE_CLICKSTART:
        return;
    case CGame::MODE_HELP:
        if (!m_keyStates[Key_F1])
        {
            // keyup
            m_keyRepeters[Key_F1] = 0;
        }
        else if (m_keyRepeters[Key_F1])
        {
            // avoid keys repeating
            return;
        }
        else
        {
            m_game->setMode(CGame::MODE_PLAY);
            m_keyRepeters[Key_F1] = 1;
        }
        return;
    case CGame::MODE_TITLE:
        manageTitleScreen();
        return;
    case CGame::MODE_OPTIONS:
        manageOptionScreen();
        return;
    case CGame::MODE_USERSELECT:
        manageUserMenu();
        return;
    case CGame::MODE_PLAY:
        manageGamePlay();
        return;
    case CGame::MODE_LEVEL_SUMMARY:
        manageLevelSummary();
        return;
    case CGame::MODE_SKLLSELECT:
        manageSkillMenu();
        return;
    case CGame::MODE_NEW_INPUTNAME:
        return;
    case CGame::MODE_TEST:
        return;
    }
}

void CGameMixin::moveCamera()
{
    const CMap &map = CGame::getMap();
    const int maxRows = getHeight() / TILE_SIZE;
    const int maxCols = getWidth() / TILE_SIZE;
    const int rows = std::min(maxRows, map.height());
    const int cols = std::min(maxCols, map.width());

    const int mx = m_cx / 2;
    const int my = m_cy / 2;
    const int x = m_game->player().x() - mx;
    const int y = m_game->player().y() - my;

    const int hDeadZone = cols / 2 - 1;
    const int vDeadZone = rows / 2 - 1;

    if (m_cy > 0 && y < vDeadZone)
        --m_cy;
    else if (m_cy < (map.height() - rows) * 2 &&
             y > rows - vDeadZone)
        ++m_cy;
    else if (m_cx > 0 && x < hDeadZone)
        --m_cx;
    else if (m_cx < (map.width() - cols) * 2 &&
             x > cols - hDeadZone)
        ++m_cx;
}

void CGameMixin::manageGamePlay()
{
    CGame &game = *m_game;
    uint8_t joyState[JOY_AIMS];
    // merge joystick and virtual joystick
    for (size_t i = 0; i < sizeof(joyState); ++i)
    {
        joyState[i] = m_joyState[i] | m_vjoyState[i];
    }

    if (m_recorder->isRecording())
    {
        m_recorder->append(joyState);
    }
    else if (m_recorder->isReading())
    {
        if (!m_recorder->get(joyState))
        {
            stopRecorder();
        }
    }
    if (m_gameMenuCooldown)
    {
        --m_gameMenuCooldown;
    }
    else if (m_keyStates[Key_Escape] ||
             m_buttonState[BUTTON_START])
    {
        stopRecorder();
        m_gameMenuCooldown = GAME_MENU_COOLDOWN;
        m_prompt = PROMPT_NONE;
        m_paused = false;
        m_keyRepeters[Key_Escape] = 0;
        toggleGameMenu();
        return;
    }

    if (m_gameMenuActive)
    {
        // disable loop while menu is active
        manageGameMenu();
        return;
    }

    if (!m_paused && handlePrompts())
    {
        stopRecorder();
        return;
    }

    if (m_paused)
    {
        stopRecorder();
        return;
    }

    manageCurrentEvent();
    manageTimer();
    game.purgeSfx();
    game.decTimers();
    if (m_ticks % game.playerSpeed() == 0 && game.closusureTimer())
    {
        // decrease closure timer
        game.decClosure();
    }
    else if (m_ticks % game.playerSpeed() == 0 &&
             !game.isPlayerDead() &&
             !game.isFrozen())
    {
        if (game.managePlayer(joyState) != AIM_NONE)
        {
            game.stats().set(S_IDLE_TIME, 0);
        }
        else
        {
            int &idleTime = game.stats().inc(S_IDLE_TIME);
            if (idleTime >= MAX_IDLE_CYCLES)
                idleTime = 0;
        }
    }

    if (m_ticks % cameraSpeed() == 0)
    {
        moveCamera();
    }

    if (!game.isFrozen() && m_ticks % 3 == 0)
    {
        if (!game.isClosure())
        {
            const int hurtStage = game.stats().get(S_PLAYER_HURT);
            if (game.health() < m_healthRef && hurtStage == CGame::HurtNone)
            {
                game.stats().set(S_PLAYER_HURT, CGame::HurtStart);
            }
            else if (*(reinterpret_cast<uint32_t *>(joyState)))
            {
                m_playerFrameOffset = (m_playerFrameOffset + 1) & PLAYER_STD_FRAMES;
            }
            else
            {
                m_playerFrameOffset = 0;
            }

            if (game.health() >= m_healthRef)
                game.stats().dec(S_PLAYER_HURT);
            m_healthRef = game.health();
        }
        else
        {
            m_playerFrameOffset = (m_playerFrameOffset + 1) % PLAYER_STD_FRAMES;
        }
    }

    if (m_ticks % 3 == 0)
        m_animator->animate();

    game.manageMonsters(m_ticks);
    game.manageBosses(m_ticks);
    const uint16_t exitKey = m_game->getMap().states().getU(POS_EXIT);
    if (game.isClosure())
    {
        stopRecorder();
        if (game.closusureTimer() != 0)
        {
            return;
        }

        if (game.isPlayerDead())
        {
            game.killPlayer();
            if (!game.isGameOver())
            {
                restartLevel();
            }
            else
            {
                startCountdown(COUNTDOWN_INTRO);
                game.setMode(CGame::MODE_GAMEOVER);
                changeMoodMusic(CGame::MODE_GAMEOVER);
            }
        }

        const bool isChute = m_game->stats().get(S_CHUTE) != 0;
        if (isChute)
        {
            clearButtonStates();
            clearJoyStates();
            nextLevel();
            m_game->setMode(CGame::MODE_CHUTE);
            startCountdown(1 * COUNTDOWN_INTRO);
            return;
        }

        if (!game.isGameOver() && !game.goalCount())
        {
            if (exitKey == 0 || m_game->player().pos() == CMap::toPos(exitKey))
            {
                if (m_summaryEnabled)
                {
                    clearButtonStates();
                    clearJoyStates();
                    initLevelSummary();
                    m_game->setMode(CGame::MODE_LEVEL_SUMMARY);
                    startCountdown(1 * COUNTDOWN_INTRO);
                }
                else
                {
                    nextLevel();
                }
            }
        }
    }
    else if (game.goalCount() == 0 && exitKey != 0)
    {
        game.checkClosure();
    }
}

void CGameMixin::nextLevel()
{
    stopRecorder();
    m_healthRef = 0;
    m_game->nextLevel();
    sanityTest();
    beginLevelIntro(CGame::MODE_LEVEL_INTRO);
    openMusicForLevel(m_game->level());
}

void CGameMixin::restartLevel()
{
    m_game->restartLevel();
    beginLevelIntro(CGame::MODE_RESTART);
    changeMoodMusic(CGame::MODE_RESTART);
}

void CGameMixin::restartGame()
{
    m_paused = false;
    m_game->restartGame();
    sanityTest();
    beginLevelIntro(CGame::MODE_LEVEL_INTRO);
    setupTitleScreen();
}

void CGameMixin::startCountdown(int f)
{
    m_countdown = f * INTRO_DELAY;
}

void CGameMixin::init(CMapArch *maparch, const int index)
{
    if (!m_assetPreloaded)
    {
        preloadAssets();
        m_assetPreloaded = true;
    }
    m_maparch = maparch;
    m_game->setMapArch(maparch);
    m_game->setLevel(index);
    sanityTest();
    beginLevelIntro(CGame::MODE_LEVEL_INTRO);
}

void CGameMixin::changeZoom()
{
    setZoom(!m_zoom);
}

void CGameMixin::setZoom(bool zoom)
{
    m_zoom = zoom;
}

bool CGameMixin::isWithin(int val, int min, int max)
{
    return val >= min && val <= max;
}

int CGameMixin::rankUserScore()
{
    const int score = m_game->score();
    if (score <= m_hiscores[MAX_SCORES - 1].score)
    {
        return INVALID;
    }

    int32_t i;
    for (i = 0; i < static_cast<int32_t>(MAX_SCORES); ++i)
    {
        if (score > m_hiscores[i].score)
        {
            break;
        }
    }

    for (int32_t j = static_cast<int32_t>(MAX_SCORES - 2); j >= i; --j)
    {
        m_hiscores[j + 1] = m_hiscores[j];
    }

    m_hiscores[i].score = m_game->score();
    m_hiscores[i].level = m_game->level() + 1;
    memset(m_hiscores[i].name, 0, sizeof(m_hiscores[i].name));
    return i;
}

void CGameMixin::drawScores(CFrame &bitmap)
{
    int scaleX = 2;
    int scaleY = 2;
    if (getWidth() > 450)
    {
        scaleX = 4;
    }
    else if (getWidth() > 350)
    {
        scaleX = 3;
    }
    if (getHeight() >= 450)
    {
        scaleY = 3;
    }
    if (getHeight() >= 350)
    {
        scaleY = 3;
    }
    else if (getHeight() >= 300)
    {
        scaleY = 2;
    }

    bitmap.fill(BLACK);
    char t[50];
    int y = 1;
    strncpy(t, "HALL OF HEROES", sizeof(t));
    int x = (getWidth() - strlen(t) * scaleX * FONT_SIZE) / 2;
    drawFont(bitmap, x, y * FONT_SIZE, t, WHITE, BLACK, scaleX, scaleY);
    y += scaleX;
    strncpy(t, std::string(strlen(t), '=').c_str(), sizeof(t) - 1);
    x = (getWidth() - strlen(t) * scaleX * FONT_SIZE) / 2;
    drawFont(bitmap, x, y * FONT_SIZE, t, WHITE, BLACK, scaleX, scaleY);
    y += scaleX;

    for (int i = 0; i < static_cast<int>(MAX_SCORES); ++i)
    {
        Color color = i & INTERLINES ? LIGHTGRAY : DARKGRAY;
        if (m_recordScore && m_scoreRank == i)
        {
            color = YELLOW;
        }
        else if (m_scoreRank == i)
        {
            color = CYAN;
        }
        bool showCaret = (color == YELLOW) && (m_ticks & CARET_SPEED);
        snprintf(t, sizeof(t), " %.8d %.2d %s%c",
                 m_hiscores[i].score,
                 m_hiscores[i].level,
                 m_hiscores[i].name,
                 showCaret ? CHARS_CARET : '\0');
        drawFont(bitmap, 1, y * FONT_SIZE, t, color, BLACK, scaleX / 2, scaleY / 2);
        y += scaleX / 2;
    }

    y += scaleX / 2;
    if (m_scoreRank == INVALID)
    {
        strncpy(t, " SORRY, YOU DIDN'T QUALIFY.", sizeof(t));
        drawFont(bitmap, 0, y * FONT_SIZE, t, YELLOW, BLACK, scaleX / 2, scaleY / 2);
    }
    else if (m_recordScore)
    {
        strncpy(t, "PLEASE TYPE YOUR NAME AND PRESS ENTER.", sizeof(t));
        x = (getWidth() - strlen(t) * FONT_SIZE) / 2;
        drawFont(bitmap, x, y++ * FONT_SIZE, t, YELLOW, BLACK, scaleX / 2, scaleY / 2);
    }
}

void CGameMixin::drawHelpScreen(CFrame &bitmap)
{
    bitmap.fill(BLACK);
    int y = 0;
    for (size_t i = 0; i < m_helptext.size(); ++i)
    {
        const char *p = m_helptext[i].c_str();
        int x = 0;
        Color color = WHITE;
        if (p[0] == '~')
        {
            ++p;
            color = YELLOW;
        }
        else if (p[0] == '!')
        {
            ++p;
            x = (getWidth() - strlen(p) * FONT_SIZE) / 2;
        }
        drawFont(bitmap, x, y * FONT_SIZE, p, color);
        ++y;
    }
}

bool CGameMixin::handlePrompts()
{
    auto result = m_prompt != PROMPT_NONE;
    if (m_prompt != PROMPT_NONE && m_keyStates[Key_N])
    {
        m_prompt = PROMPT_NONE;
    }
    else if (m_keyStates[Key_Y])
    {
        if (m_prompt == PROMPT_ERASE_SCORES)
        {
            clearScores();
            saveScores();
        }
        else if (m_prompt == PROMPT_RESTART_GAME)
        {
            restartGame();
        }
        else if (m_prompt == PROMPT_LOAD)
        {
            load();
        }
        else if (m_prompt == PROMPT_SAVE)
        {
            save();
        }
        else if (m_prompt == PROMPT_HARDCORE)
        {
            m_game->setLives(1);
        }
        else if (m_prompt == PROMPT_TOGGLE_MUSIC)
        {
            m_musicMuted ? startMusic() : stopMusic();
            m_musicMuted = !m_musicMuted;
        }
        m_prompt = PROMPT_NONE;
    }
    return result;
}

void CGameMixin::handleFunctionKeys()
{
    for (int k = Key_F1; k <= Key_F12; ++k)
    {
        if (!m_keyStates[k])
        {
            // keyup
            m_keyRepeters[k] = 0;
            continue;
        }
        else if (m_keyRepeters[k])
        {
            // avoid keys repeating
            continue;
        }
        if (m_paused && k != Key_F4)
        {
            // don't handle any other
            // hotkeys while paused
            continue;
        }

        if (m_game->mode() == CGame::MODE_PLAY)
        {
            handleFunctionKeys_Game(k);
        }
        else
        {
            handleFunctionKeys_General(k);
        }
    }
}

void CGameMixin::handleFunctionKeys_Game(int k)
{
    switch (k)
    {
    case Key_F1:
        m_game->setMode(CGame::MODE_HELP);
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
    case Key_F2: // restart game
        m_prompt = PROMPT_RESTART_GAME;
        break;
    case Key_F3: // erase scores
        m_prompt = PROMPT_ERASE_SCORES;
        break;
    case Key_F4:
        m_paused = !m_paused;
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;

#ifndef __EMSCRIPTEN__
    case Key_F5:
        toggleFullscreen();
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
    case Key_F6:
        playbackGame();
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
    case Key_F7:
        recordGame();
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
    case Key_F8:
        takeScreenshot();
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
#endif
    case Key_F9:
        m_prompt = PROMPT_LOAD;
        break;
    case Key_F10:
        m_prompt = PROMPT_SAVE;
        break;
    case Key_F11:
        m_prompt = PROMPT_TOGGLE_MUSIC;
        break;
    case Key_F12:
        m_prompt = PROMPT_HARDCORE;
    }
}

void CGameMixin::handleFunctionKeys_General(int k)
{
#ifdef __EMSCRIPTEN__
    (void)k;
#else
    switch (k)
    {
    case Key_F8:
        takeScreenshot();
        m_keyRepeters[k] = KEY_NO_REPETE;
        break;
    }
#endif
}

bool CGameMixin::handleInputString(char *inputDest, const size_t limit)
{
    auto range = [](uint16_t keyCode, uint16_t start, uint16_t end)
    {
        return keyCode >= start && keyCode <= end;
    };

    for (int k = 0; k < Key_Count; ++k)
    {
        m_keyRepeters[k] ? --m_keyRepeters[k] : 0;
    }

    for (int k = 0; k < Key_Count; ++k)
    {
        if (!m_keyStates[k])
        {
            m_keyRepeters[k] = 0;
            continue;
        }
        else if (m_keyRepeters[k])
        {
            continue;
        }
        char c = 0;
        if (range(k, Key_0, Key_9))
        {
            c = k + '0' - Key_0;
        }
        else if (range(k, Key_A, Key_Z))
        {
            c = k + 'A' - Key_A;
        }
        else if (k == Key_Space)
        {
            c = ' ';
        }
        else if (k == Key_Period)
        {
            c = '.';
        }
        else if (k == Key_BackSpace)
        {
            m_keyRepeters[k] = KEY_REPETE_DELAY;
            int i = strlen(inputDest);
            if (i > 0)
            {
                inputDest[i - 1] = '\0';
            }
            continue;
        }
        else if (k == Key_Enter)
        {
            return true;
        }
        else
        {
            // don't handle any other keys
            m_keyRepeters[k] = 0;
            continue;
        }
        if (strlen(inputDest) == limit - 1)
        {
            // already at maxlenght
            continue;
        }
        m_keyRepeters[k] = KEY_REPETE_DELAY;
        char s[2] = {c, 0};
        strncat(inputDest, s, limit);
    }
    return false;
}

bool CGameMixin::inputPlayerName()
{
    const int j = m_scoreRank;
    return handleInputString(m_hiscores[j].name,
                             sizeof(m_hiscores[j].name));
}

void CGameMixin::enableHiScore()
{
    m_hiscoreEnabled = true;
}

void CGameMixin::clearScores()
{
    memset(m_hiscores, 0, sizeof(m_hiscores));
}

void CGameMixin::clearKeyStates()
{
    memset(m_keyStates, 0, sizeof(m_keyStates));
    memset(m_keyRepeters, 0, sizeof(m_keyRepeters));
}

void CGameMixin::clearJoyStates()
{
    memset(m_joyState, 0, sizeof(m_joyState));
    memset(m_vjoyState, 0, sizeof(m_vjoyState));
}

bool CGameMixin::read(IFile &sfile, std::string &name)
{
    auto readfile = [&sfile](auto ptr, auto size)
    {
        return sfile.read(ptr, size) == 1;
    };
    if (!m_game->read(sfile))
    {
        return false;
    }
    clearButtonStates();
    clearJoyStates();
    clearKeyStates();
    clearVisualStates();
    m_paused = false;
    m_prompt = PROMPT_NONE;
    _R(&m_ticks, sizeof(m_ticks));
    _R(&m_playerFrameOffset, sizeof(m_playerFrameOffset));
    _R(&m_healthRef, sizeof(m_healthRef));
    _R(&m_countdown, sizeof(m_countdown));

    size_t ptr = 0;
    sfile.seek(SAVENAME_PTR_OFFSET);
    // fseek(sfile, SAVENAME_PTR_OFFSET, SEEK_SET);
    _R(&ptr, sizeof(uint32_t));
    sfile.seek(ptr);
    // fseek(sfile, ptr, SEEK_SET);
    size_t size = 0;
    _R(&size, sizeof(uint16_t));
    std::vector<char> tmp(size);
    _R(tmp.data(), size);
    name = tmp.data();
    return true;
}

bool CGameMixin::write(IFile &tfile, const std::string &name)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == 1;
    };
    m_game->write(tfile);
    _W(&m_ticks, sizeof(m_ticks));
    _W(&m_playerFrameOffset, sizeof(m_playerFrameOffset));
    _W(&m_healthRef, sizeof(m_healthRef));
    _W(&m_countdown, sizeof(m_countdown));

    const size_t ptr = tfile.tell();
    const size_t size = name.size();
    _W(&size, sizeof(uint16_t));
    _W(name.c_str(), name.size());
    // save EOF offset
    auto offset = tfile.tell();
    tfile.seek(SAVENAME_PTR_OFFSET);
    // fseek(tfile, SAVENAME_PTR_OFFSET, SEEK_SET);
    _W(&ptr, sizeof(uint32_t));
    // return offset to EOF
    tfile.seek(offset);
    return true;
}

void CGameMixin::drawPreScreen(CFrame &bitmap)
{
    const char t[] = "CLICK TO START";
    const int x = (getWidth() - strlen(t) * FONT_SIZE) / 2;
    const int y = (getHeight() - FONT_SIZE) / 2;
    bitmap.fill(BLACK);
    drawFont(bitmap, x, y, t, WHITE);
}

void CGameMixin::setSkill(uint8_t skill)
{
    m_game->setSkill(skill);
}

void CGameMixin::clearButtonStates()
{
    memset(m_buttonState, 0, sizeof(m_buttonState));
}

void CGameMixin::fazeScreen(CFrame &bitmap, const int bitShift)
{
    const uint32_t colorFilter = fazFilter(bitShift);
    for (int y = 0; y < bitmap.height(); ++y)
    {
        for (int x = 0; x < bitmap.width(); ++x)
        {
            bitmap.at(x, y) =
                ((bitmap.at(x, y) >> bitShift) & colorFilter) | ALPHA;
        }
    }
}

void CGameMixin::flashScreen(CFrame &bitmap)
{
    for (int y = 0; y < bitmap.height(); ++y)
    {
        for (int x = 0; x < bitmap.width(); ++x)
        {
            const auto &rgba = bitmap.at(x, y);
            if (rgba & 0xffffff)
                bitmap.at(x, y) =
                    rgba | 0xffffff;
        }
    }
}

void CGameMixin::stopRecorder()
{
    if (!m_recorder->isStopped())
    {
        m_recorder->stop();
    }
}

void CGameMixin::recordGame()
{
    if (m_recorder->isRecording())
        return;
    m_recorder->stop();
    const std::string name = "test";
    const std::string path = "test.rec";
    if (!m_recorderFile.open(path.c_str(), "wb"))
    {
        LOGE("cannot create: %s", path.c_str());
        return;
    }
    write(m_recorderFile, name);
    m_recorder->start(&m_recorderFile, true);
}

void CGameMixin::playbackGame()
{
    m_recorder->stop();
    std::string name;
    const std::string path = "test.rec";
    if (!m_recorderFile.open(path.c_str()))
    {
        LOGE("cannot read: %s", path.c_str());
        return;
    }
    read(m_recorderFile, name);
    openMusicForLevel(m_game->level());
    m_recorder->start(&m_recorderFile, false);
}

void CGameMixin::plotLine(CFrame &bitmap, int x0, int y0, const int x1, const int y1, const Color color)
{
    auto dx = abs(x1 - x0);
    auto sx = x0 < x1 ? 1 : -1;
    auto dy = -abs(y1 - y0);
    auto sy = y0 < y1 ? 1 : -1;
    auto error = dx + dy;
    while (true)
    {
        bitmap.at(x0, y0) = color;
        auto e2 = 2 * error;
        if (e2 >= dy)
        {
            if (x0 == x1)
                break;
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            if (y0 == y1)
                break;
            error += dx;
            y0 += sy;
        }
    }
}

int CGameMixin::cameraSpeed() const
{
    return std::max(m_game->playerSpeed() - 1, 1);
}

void CGameMixin::setCameraMode(const int mode)
{
    m_cameraMode = mode & 1;
}

void CGameMixin::drawEventText(CFrame &bitmap)
{
    if (m_currentEvent != EVENT_NONE)
    {
        const int baseY = bitmap.height() - 4;
        const message_t message = getEventText(baseY);
        const size_t maxLines = sizeof(message.lines) / sizeof(message.lines[0]);
        for (size_t i = 0; i < maxLines; ++i)
        {
            const auto &line = std::move(message.lines[i]);
            if (line.size() == 0)
                break;
            const int x = (getWidth() - line.size() * FONT_SIZE * message.scaleX) / 2;
            const int y = message.baseY - FONT_SIZE * message.scaleY + i * 10;
            drawFont(bitmap, x, y, line.c_str(), message.color, CLEAR, message.scaleX, message.scaleY);
        }
    }
}

CGameMixin::message_t CGameMixin::getEventText(const int baseY)
{
    if (m_currentEvent == EVENT_SECRET)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = CYAN,
            .lines{"SECRET !"},
        };
    }
    else if (m_currentEvent == EVENT_PASSAGE)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = LIGHTGRAY,
            .lines{"PASSAGE"},
        };
    }
    else if (m_currentEvent == EVENT_EXTRA_LIFE)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = GREEN,
            .lines{"EXTRA LIFE"},
        };
    }
    else if (m_currentEvent == EVENT_SUGAR_RUSH)
    {
        Color color;
        if ((m_ticks >> 3) & 1)
            color = LIME;
        else
            color = ORANGE;
        return {
            .scaleX = 2,
            .scaleY = 2,
            .baseY = baseY,
            .color = color,
            .lines{"SUGAR RUSH"},
        };
    }
    else if (m_currentEvent == EVENT_GOD_MODE)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = BLACK,
            .lines{"GOD MODE!"},
        };
    }
    else if (m_currentEvent == EVENT_SUGAR)
    {
        char tmp[40];
        // const auto sugarLevel = m_game->stats().get(S_SUGAR_LEVEL);
        snprintf(tmp, sizeof(tmp), "YUMMY %d/%d", m_game->sugar(), CGame::MAX_SUGAR_RUSH_LEVEL);
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = PURPLE,
            .lines{tmp},
        };
    }
    else if (RANGE(m_currentEvent, MSG0, MSGF))
    {
        const std::string tmp = m_game->getMap().states().getS(m_currentEvent);
        const auto list = split(tmp, '\n');
        const std::string &line1 = list[0];
        const std::string &line2 = list.size() > 1 ? list[1] : "";
        const std::string &line3 = list.size() > 2 ? list[2] : "";
        const int y = list.size() == 0 ? baseY - 14 : baseY + -8 * (std ::min(list.size(), (size_t)3));
        return {
            .scaleX = 1,
            .scaleY = 1,
            .baseY = y,
            .color = DARKGRAY,
            .lines{line1, line2, line3},
        };
    }
    else if (m_currentEvent == EVENT_RAGE)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = DARKORANGE,
            .lines{"RAGE !!!"},
        };
    }
    else if (m_currentEvent == EVENT_FREEZE)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = WHITE,
            .lines{"FREEZE TRAP"},
        };
    }
    else if (m_currentEvent == EVENT_TRAP)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = RED,
            .lines{"TRAP"},
        };
    }
    else if (m_currentEvent == EVENT_EXIT_OPENED)
    {
        const char *text = (m_ticks >> 3) & 1 ? "EXIT DOOR IS OPENED" : "";
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY - (int)TILE_SIZE,
            .color = SEAGREEN,
            .lines{text},
        };
    }
    else if (m_currentEvent == EVENT_SHIELD)
    {
        char tmp[40];
        snprintf(tmp, sizeof(tmp), "SHIELD %d%%", 25 * m_game->stats().get(S_SHIELD));
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = LIME,
            .lines{tmp},
        };
    }
    else if (m_currentEvent == EVENT_BOAT)
    {
        return {
            .scaleX = 2,
            .scaleY = 1,
            .baseY = baseY,
            .color = BROWN,
            .lines{"BOAT FOUND"},
        };
    }
    else
    {
        LOGW("unhandled event: 0x%.2x", m_currentEvent);
        return message_t{};
    }
}

int CGameMixin::tickRate()
{
    return TICK_RATE;
}

void CGameMixin::manageCurrentEvent()
{
    if (m_eventCountdown > 0)
    {
        --m_eventCountdown;
    }
    else if (m_currentEvent == EVENT_NONE)
    {
        m_currentEvent = m_game->getEvent();
        if (m_currentEvent >= MSG0)
        {
            m_eventCountdown = MSG_COUNTDOWN_DELAY;
        }
        else if (m_currentEvent == EVENT_TRAP || m_currentEvent == EVENT_FREEZE)
        {
            m_eventCountdown = TRAP_MSG_COUNTDOWN_DELAY;
        }
        else if (m_currentEvent)
        {
            m_eventCountdown = EVENT_COUNTDOWN_DELAY;
        }
    }
    else
    {
        m_currentEvent = EVENT_NONE;
    }
}

void CGameMixin::manageTimer()
{
    CGame &game = *m_game;
    if (m_timer)
    {
        --m_timer;
    }
    else
    {
        game.incTimeTaken();
        m_timer = TICK_RATE;
        CStates &states = game.getMap().states();
        uint16_t timeout = states.getU(TIMEOUT);
        if (timeout == 1)
        {
            game.killPlayer();
            if (game.isGameOver())
            {
                game.setMode(CGame::MODE_GAMEOVER);
                changeMoodMusic(CGame::MODE_GAMEOVER);
            }
            else
            {
                beginLevelIntro(CGame::MODE_TIMEOUT);
                m_timer = TICK_RATE;
            }
        }
        else if (timeout > 0)
        {
            if (timeout <= 15)
            {
                game.playSound(SOUND_BEEP);
            }
            states.setU(TIMEOUT, timeout - 1);
        }
    }
}

/**
 * @brief Draw Sugar Meter and Sugar SFX
 *
 * @param bitmap
 * @param bx
 */
void CGameMixin::drawSugarMeter(CFrame &bitmap, const int bx)
{
    constexpr const Color sugarColors[] = {
        RED,
        DEEPPINK,
        HOTPINK,
        ORANGE,
        PINK,
        BLACK,
        BLACK,
        YELLOW,
    };
    const bool updateNow = ((m_ticks >> 1) & 1);
    const int MAX_FX_COLOR = sizeof(sugarColors) / sizeof(sugarColors[0]) - 1;
    CGame &game = *m_game;
    const int sugar = game.sugar();
    if ((sugar > m_visualStates.rSugar || game.hasExtraSpeed()) &&
        !m_visualStates.sugarFx)
    {
        if (sugar > 0)
            m_visualStates.sugarCubes[sugar - 1] = MAX_FX_COLOR;
        m_visualStates.sugarFx = MAX_FX_COLOR;
    }
    else if (m_visualStates.sugarFx && updateNow)
    {
        --m_visualStates.sugarFx;
    }
    m_visualStates.rSugar = sugar;
    for (int i = 0; i < (int)CGame::MAX_SUGAR_RUSH_LEVEL; ++i)
    {
        const rect_t rect{
            .x = bx * (int)FONT_SIZE + i * 5,
            .y = Y_STATUS + 2,
            .width = 4,
            .height = 4};
        if (static_cast<int>(i) < sugar || game.hasExtraSpeed())
        {
            if (sugar < SUGAR_CUBES && !game.hasExtraSpeed())
            {
                const uint8_t &idx = m_visualStates.sugarCubes[i];
                drawRect(bitmap, rect, sugarColors[idx], true);
            }
            else
            {
                drawRect(bitmap, rect, sugarColors[m_visualStates.sugarFx], true);
            }
        }
        else
        {
            drawRect(bitmap, rect, WHITE, false);
        }
    }

    // indicate sugar level
    const int x = bx * (int)FONT_SIZE;
    const int y = Y_STATUS + 2 + (int)FONT_SIZE;
    char tmp[20];
    snprintf(tmp, sizeof(tmp), "Lvl %d", m_game->stats().get(S_SUGAR_LEVEL) + 1);
    drawFont6x6(bitmap, x, y, tmp, WHITE, CLEAR);

    if (updateNow)
    {
        for (int i = 0; i < SUGAR_CUBES; ++i)
        {
            if (m_visualStates.sugarCubes[i])
                --m_visualStates.sugarCubes[i];
        }
    }
}

/**
 * @brief Calculate Special Frame for Special Case Animations (Sprites)
 *
 * @param sprite
 * @return CFrame*
 */
CFrame *CGameMixin::calcSpecialFrame(const sprite_t &sprite)
{
    if (RANGE(sprite.attr, ATTR_IDLE_MIN, ATTR_IDLE_MAX))
    {
        CFrameSet &tiles = *m_tiles;
        return tiles[sprite.tileID];
    }
    int saim = 0;
    if (sprite.tileID < TILES_TOTAL_COUNT)
    {
        // safeguard
        saim = sprite.aim;
        const TileDef &def = getTileDef(sprite.tileID);
        if (def.type == TYPE_DRONE)
        {
            saim &= 1;
        }
    }
    CFrameSet &animz = *m_animz;
    const animzInfo_t info = m_animator->getSpecialInfo(sprite.tileID);
    if (sprite.tileID == SFX_FLAME)
    {
        // LOGI("info.base=%u", info.base);
    }
    return animz[saim * info.frames + info.base + info.offset];
}

/**
 * @brief Set Texture Width
 *
 * @param w
 */
void CGameMixin::setWidth(int w)
{
    _WIDTH = w;
}

/**
 * @brief Set Texture Height
 *
 * @param h
 */
void CGameMixin::setHeight(int h)
{
    _HEIGHT = h;
}

/**
 * @brief Draw Player Health Bar
 *
 * @param bitmap
 * @param isPlayerHurt
 */
void CGameMixin::drawHealthBar(CFrame &bitmap, const bool isPlayerHurt)
{
    const uint32_t color = m_game->isGodMode() ? WHITE : isPlayerHurt ? PINK
                                                                      : RED;
    auto drawHearth = [&bitmap, color](auto bx, auto by, auto health)
    {
        const uint8_t *heart = getCustomChars() + (CHARS_HEART - CHARS_CUSTOM) * FONT_SIZE;
        for (uint32_t y = 0; y < FONT_SIZE; ++y)
        {
            for (uint32_t x = 0; x < FONT_SIZE; ++x)
            {
                const uint8_t bit = heart[y] & (1 << x);
                if (bit)
                    bitmap.at(bx + x, by + y) = x < (uint32_t)health ? color : BLACK;
            }
        }
    };

    // draw health bar
    CGame &game = *m_game;
    if (m_healthBar == HEALTHBAR_HEARTHS)
    {
        int step = FONT_SIZE;
        const int maxHealth = game.maxHealth() / 2 / FONT_SIZE;
        int health = game.health() / 2;
        int bx = 2;
        int by = bitmap.height() - 12;
        for (int i = 0; i < maxHealth; ++i)
        {
            drawHearth(bx, by, health > 0 ? health : 0);
            bx += FONT_SIZE;
            health -= step;
        }
    }
    else if (m_healthBar == HEALTHBAR_CLASSIC)
    {
        const Color hpColor = game.isGodMode() ? WHITE : isPlayerHurt ? PINK
                                                                      : LIME;
        const int hpWidth = std::min(game.health() / 2, bitmap.width() - 4);
        drawRect(bitmap, rect_t{4, bitmap.height() - 12, hpWidth, 8},
                 hpColor, true);
        drawRect(bitmap, rect_t{4, bitmap.height() - 12, hpWidth, 8},
                 WHITE, false);
    }
}

/**
 * @brief Draw Top Line Status Bar Overlay on Game Screen
 *
 * @param bitmap
 * @param visualcues
 */
void CGameMixin::drawGameStatus(CFrame &bitmap, const visualCues_t &visualcues)
{
    CGame &game = *m_game;
    char tmp[32];
    if (m_paused)
    {
        drawFont(bitmap, 0, Y_STATUS, "PRESS [F4] TO RESUME PLAYING...", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_ERASE_SCORES)
    {
        drawFont(bitmap, 0, Y_STATUS, "ERASE HIGH SCORES, CONFIRM (Y/N)?", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_RESTART_GAME)
    {
        drawFont(bitmap, 0, Y_STATUS, "RESTART GAME, CONFIRM (Y/N)?", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_LOAD)
    {
        drawFont(bitmap, 0, Y_STATUS, "LOAD PREVIOUS SAVEGAME, CONFIRM (Y/N)?", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_SAVE)
    {
        drawFont(bitmap, 0, Y_STATUS, "SAVE GAME, CONFIRM (Y/N)?", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_HARDCORE)
    {
        drawFont(bitmap, 0, Y_STATUS, "HARDCORE MODE, CONFIRM (Y/N)?", LIGHTGRAY);
    }
    else if (m_prompt == PROMPT_TOGGLE_MUSIC)
    {
        drawFont(bitmap, 0, Y_STATUS,
                 m_musicMuted ? "PLAY MUSIC, CONFIRM (Y/N)?"
                              : "MUTE MUSIC, CONFIRM (Y/N)?",
                 LIGHTGRAY);
    }
    else
    {
        int tx;
        int bx = 0;
        tx = snprintf(tmp, sizeof(tmp), "%.8d ", game.score());
        drawFont(bitmap, 0, Y_STATUS, tmp, WHITE);
        bx += tx;
        tx = snprintf(tmp, sizeof(tmp), "DIAMONDS %.2d ", game.goalCount());
        drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, tmp, visualcues.diamondShimmer ? DEEPSKYBLUE : YELLOW);
        bx += tx;
        tx = snprintf(tmp, sizeof(tmp), "LIVES %.2d ", game.lives());
        drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, tmp, visualcues.livesShimmer ? GREEN : PURPLE);
        bx += tx;
        if (m_recorder->isRecording())
        {
            drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, "REC!", WHITE, RED);
        }
        else if (m_recorder->isReading())
        {
            drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, "PLAY", WHITE, DARKGREEN);
        }
        if (getWidth() >= MIN_WIDTH_FULL)
            drawSugarMeter(bitmap, bx);
    }
}

/**
 * @brief Start Level Intro
 *
 * @param mode  MODE_INTRO, MODE_RESTART or MODE_TIMEOUT
 */
void CGameMixin::beginLevelIntro(CGame::GameMode mode)
{
    startCountdown(COUNTDOWN_INTRO);
    m_game->loadLevel(mode);
    centerCamera();
    clearVisualStates();
}

/**
 * @brief Clear Visual Cue Indicators reset all to zero
 *
 */
void CGameMixin::clearVisualStates()
{
    m_visualStates.rGoalCount = 0;
    m_visualStates.rLives = 0;
    m_visualStates.rSugar = 0;
    m_visualStates.sugarFx = 0;
    memset(m_visualStates.sugarCubes, '\0', sizeof(m_visualStates.sugarCubes));
}

void CGameMixin::initUI()
{
    const int BTN_SIZE = 32;
    m_ui.setMargin(FONT_SIZE);
    std::vector<button_t> buttons{
        {.id = AIM_UP, .x = BTN_SIZE, .y = 0, .width = BTN_SIZE, .height = BTN_SIZE, .text = "", .color = WHITE},
        {.id = AIM_LEFT, .x = 0, .y = BTN_SIZE, .width = BTN_SIZE, .height = BTN_SIZE, .text = "", .color = WHITE},
        {.id = AIM_DOWN, .x = BTN_SIZE, .y = BTN_SIZE, .width = BTN_SIZE, .height = BTN_SIZE, .text = "", .color = WHITE},
        {.id = AIM_RIGHT, .x = BTN_SIZE * 2, .y = BTN_SIZE, .width = BTN_SIZE, .height = BTN_SIZE, .text = "", .color = WHITE},
    };
    for (const auto &btn : buttons)
    {
        m_ui.addButton(btn);
    }
}

void CGameMixin::drawUI(CFrame &bitmap, CGameUI &ui)
{
    const int baseX = getWidth() - ui.width() - ui.margin();
    const int baseY = getWidth() - ui.height() - ui.margin();
    for (const auto &btn : ui.buttons())
    {
        int x = baseX + btn.x;
        int y = baseY + btn.y;
        drawRect(bitmap, rect_t{.x = baseX + btn.x, .y = baseY + btn.y, .width = btn.width, .height = btn.height}, static_cast<Color>(btn.color), true);
        drawFont(bitmap, x, y, btn.text.c_str(), BLACK, CLEAR, 2, 2);
        drawRect(bitmap, rect_t{.x = baseX + btn.x, .y = baseY + btn.y, .width = btn.width, .height = btn.height}, RED, false);
    }
}

int CGameMixin::whichButton(CGameUI &ui, int x, int y)
{
    const int baseX = getWidth() - ui.width() - ui.margin();
    const int baseY = getHeight() - ui.height() - ui.margin();
    for (const auto &btn : ui.buttons())
    {
        if (RANGE(x, baseX + btn.x, baseX + btn.x + btn.width) &&
            RANGE(y, baseY + btn.y, baseY + btn.y + btn.height))
        {
            return btn.id;
        }
    }
    return INVALID;
}

void CGameMixin::setQuiet(bool state)
{
    m_quiet = state;
    CGame::getGame()->setQuiet(state);
}