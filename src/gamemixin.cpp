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
#include "gamemixin.h"
#include "tilesdata.h"
#include "animzdata.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
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

#ifdef USE_QFILE
#include <QDebug>
#define printf qDebug
#endif

// Color Maps
std::unordered_map<uint32_t, uint32_t> g_annieWhiteColors = {
    {0xff233edf, 0xff96999a},
    {0xff2a20b4, 0xff7b7677},
    {0xffc45c28, 0xffac4191},
    {0xff643414, 0xff833561},
    {0xff2d1773, 0xff645c5e},
    {0xff0a6afa, 0xffbcbfc0},
};

std::unordered_map<uint32_t, uint32_t> g_annieYellowColors = {
    {0xff233edf, 0xff0aa5e5},
    {0xff2a20b4, 0xff48a3ff},
    {0xffc45c28, 0xff89e357},
    {0xff643414, 0xff7ec22e},
    {0xff342224, 0xff69a226},
    {0xff2d1773, 0xff645c5e},
    {0xff0a6afa, 0xff2dd3f6},
};

CGameMixin::CGameMixin()
{
    m_game = new CGame();
    m_animator = new CAnimator();
    m_prompt = PROMPT_NONE;
    clearJoyStates();
    clearScores();
    clearKeyStates();
    clearButtonStates();
    m_recorder = new CRecorder;
    m_eventCountdown = 0;
    m_currentEvent = EVENT_NONE;
}

CGameMixin::~CGameMixin()
{
    if (m_animator)
    {
        delete m_animator;
    }

    if (m_game)
    {
        delete m_game;
    }

    if (m_tiles)
    {
        delete m_tiles;
    }

    if (m_animz)
    {
        delete m_animz;
    }

    if (m_annie)
    {
        delete m_annie;
    }

    if (m_fontData)
    {
        delete[] m_fontData;
    }

    if (m_recorder)
    {
        delete m_recorder;
    }
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
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.len();
    const int fontSize = FONT_SIZE;
    const int fontOffset = fontSize;
    const int textSize = strlen(text);
    for (int i = 0; i < textSize; ++i)
    {
        uint8_t c = static_cast<uint8_t>(text[i]);
        const uint8_t *font = c >= CHARS_CUSTOM ? getCustomChars() + (c - CHARS_CUSTOM) * fontOffset
                                                : m_fontData + (c - ' ') * fontOffset;
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

void CGameMixin::drawRect(CFrame &frame, const Rect &rect, const Color color, bool fill)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.len();
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

void CGameMixin::drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const Rect &rect, const bool inverted, std::unordered_map<uint32_t, uint32_t> *colorMap)
{
    const int width = bitmap.len();
    uint32_t *dest = bitmap.getRGB() + x + y * width;
    if (!inverted && !colorMap)
    {
        for (int row = 0; row < rect.height; ++row)
        {
            for (int col = 0; col < rect.width; ++col)
            {
                dest[col] = tile.at(col + rect.x, row + rect.y);
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
                if (!color)
                    continue;
                if (colorMap && colorMap->count(color))
                {
                    color = (*colorMap)[color];
                }
                if (inverted)
                {
                    // color ^= 0x00ffffff;
                    const uint32_t t = ((color >> FAZ_INV_BITSHIFT) & colorFilter) | ALPHA;
                    color = t;
                }
                dest[col] = color;
            }
            dest += width;
        }
    }
}

void CGameMixin::drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const bool alpha, const bool inverted, std::unordered_map<uint32_t, uint32_t> *colorMap)
{
    const int width = bitmap.len();
    const uint32_t *tileData = tile.getRGB();
    uint32_t *dest = bitmap.getRGB() + x + y * width;
    if (alpha || inverted || colorMap)
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
                if (inverted)
                {
                    // color ^= 0x00ffffff;
                    const uint32_t t = ((color >> FAZ_INV_BITSHIFT) & colorFilter) | ALPHA;
                    color = t;
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

void CGameMixin::drawKeys(CFrame &bitmap)
{
    CGame &game = *m_game;
    CFrameSet &tiles = *m_tiles;
    int y = HEIGHT - TILE_SIZE;
    int x = WIDTH - TILE_SIZE;
    const uint8_t *keys = game.keys();
    for (size_t i = 0; i < CGame::MAX_KEYS; ++i)
    {
        uint8_t k = keys[i];
        if (k)
        {
            drawTile(bitmap, x, y, *tiles[k], true);
            x -= TILE_SIZE;
        }
    }
}

void CGameMixin::drawScreen(CFrame &bitmap)
{
    // draw viewport
    CGame &game = *m_game;
    if (m_cameraMode == CAMERA_MODE_DYNAMIC)
        drawViewPortDynamic(bitmap);
    else if (m_cameraMode == CAMERA_MODE_STATIC)
        drawViewPortStatic(bitmap);

    if (m_playerFrameOffset == PLAYER_HIT_FRAME)
    {
        bitmap.shiftLEFT(false);
        bitmap.shiftLEFT(false);
        bitmap.shiftLEFT(false);
        bitmap.shiftLEFT(false);
    }

    // draw game status
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
        tx = sprintf(tmp, "%.8d ", game.score());
        drawFont(bitmap, 0, Y_STATUS, tmp, WHITE);
        bx += tx;
        tx = sprintf(tmp, "DIAMONDS %.2d ", game.goalCount());
        drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, tmp, YELLOW);
        bx += tx;
        tx = sprintf(tmp, "LIVES %.2d ", game.lives());
        drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, tmp, PURPLE);
        bx += tx;
        if (m_recorder->isRecording())
        {
            drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, "REC!", WHITE, RED);
        }
        else if (m_recorder->isReading())
        {
            drawFont(bitmap, bx * FONT_SIZE, Y_STATUS, "PLAY", WHITE, DARKGREEN);
        }
        drawSugarMeter(bitmap, bx);
    }

    // draw bottom rect
    const Color rectBG = m_currentEvent >= MSG0 ? WHITE : DARKGRAY;
    drawRect(bitmap, Rect{0, bitmap.hei() - 16, WIDTH, TILE_SIZE}, rectBG, true);
    drawRect(bitmap, Rect{0, bitmap.hei() - 16, WIDTH, TILE_SIZE}, LIGHTGRAY, false);

    // draw current event text
    drawEventText(bitmap);

    if (m_currentEvent < MSG0)
    {
        // draw health bar
        const int hpWidth = std::min(game.health() / 2, bitmap.len() - 4);
        drawRect(bitmap, Rect{4, bitmap.hei() - 12, hpWidth, 8},
                 game.isGodMode() ? WHITE : LIME, true);
        drawRect(bitmap, Rect{4, bitmap.hei() - 12, hpWidth, 8},
                 WHITE, false);

        // draw keys
        drawKeys(bitmap);
    }

    // draw timeout
    drawTimeout(bitmap);
}

void CGameMixin::drawTimeout(CFrame &bitmap)
{
    CMap *map = &m_game->getMap();
    CStates &states = map->states();
    const uint16_t timeout = states.getU(TIMEOUT);
    if (timeout)
    {
        char tmp[16];
        sprintf(tmp, "%.d", timeout);
        const bool lowTime = timeout <= 15;
        const int scaleX = !lowTime ? 3 : 5;
        const int scaleY = !lowTime ? 4 : 5;
        const int x = WIDTH - scaleX * FONT_SIZE * strlen(tmp) - FONT_SIZE;
        const int y = 2 * FONT_SIZE;
        Color color = YELLOW;
        if (lowTime && (m_ticks >> 3) & 1)
        {
            color = ORANGE;
        }
        drawFont(bitmap, x, y, tmp, color, CLEAR, scaleX, scaleY);
    }
}

CFrame *CGameMixin::tile2Frame(const uint8_t tileID, bool &inverted, std::unordered_map<uint32_t, uint32_t> *&colorMap)
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
        CFrameSet &annie = *m_annie;
        const int aim = game.playerConst().getAim();
        tile = annie[aim * PLAYER_FRAMES + m_playerFrameOffset];
        inverted = (m_playerFrameOffset & PLAYER_HIT_FRAME) == PLAYER_HIT_FRAME;
        if (m_game->hasExtraSpeed())
        {
            colorMap = &g_annieYellowColors;
        }
        else if (m_game->isGodMode())
        {
            colorMap = &g_annieWhiteColors;
        }
    }
    else
    {
        uint16_t j = m_animator->at(tileID);
        if (j == NO_ANIMZ)
        {
            CFrameSet &tiles = *m_tiles;
            tile = tiles[tileID];
        }
        else
        {
            CFrameSet &animz = *m_animz;
            tile = animz[j];
        }
    }
    return tile;
}

void CGameMixin::gatherSprites(std::vector<sprite_t> &sprites, const cameraContext_t &context)
{
    CGame &game = *m_game;
    CMap *map = &m_game->getMap();
    const int maxRows = HEIGHT / TILE_SIZE;
    const int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    const int mx = context.mx; // m_cx / 2; // this is wrong for static camera
    const int ox = context.ox; //  m_cx & 1; // TODO: fix this
    const int my = context.my; // m_cy / 2;
    const int oy = context.oy; // m_cy & 1;
    const std::vector<CActor> &monsters = game.getMonsters();
    for (const auto &monster : monsters)
    {
        const uint8_t &tileID = map->at(monster.getX(), monster.getY());
        if (monster.within(mx, my, mx + cols + ox, my + rows + oy) &&
            m_animator->isSpecialCase(tileID))
        {
            sprites.push_back(
                {.x = monster.getX(),
                 .y = monster.getY(),
                 .tileID = tileID,
                 .aim = monster.getAim()});
        }
    }

    const std::vector<sfx_t> &sfxAll = m_game->getSfx();
    for (const auto &sfx : sfxAll)
    {
        if (sfx.within(mx, my, mx + cols + ox, my + rows + oy))
        {
            sprites.push_back({
                .x = sfx.x,
                .y = sfx.y,
                .tileID = sfx.sfx,
                .aim = AIM_NONE,
            });
        }
    }
}

void CGameMixin::drawViewPortDynamic(CFrame &bitmap)
{
    CMap *map = &m_game->getMap();
    const int maxRows = HEIGHT / TILE_SIZE;
    const int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
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
            bool inverted = false;
            std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr;
            CFrame *tile = tile2Frame(tileID, inverted, colorMap);
            if (tile)
            {
                if (firstX || firstY || lastX || lastY)
                {
                    Rect rect{
                        .x = !firstX ? 0 : halfOffset,
                        .y = !firstY ? 0 : halfOffset,
                        .width = !(firstX || lastX) ? tileSize : halfOffset,
                        .height = !(firstY || lastY) ? tileSize : halfOffset,
                    };
                    drawTile(bitmap,
                             !firstX ? px : 0,
                             !firstY ? py : 0,
                             *tile, rect, inverted, colorMap);
                }
                else
                {
                    drawTile(bitmap, px, py, *tile, false, inverted, colorMap);
                }
            }
            px += TILE_SIZE;
        }
        py += TILE_SIZE;
    }

    /////////////////////////////////////////////////////////////////////////////
    // overlay special case monsters

    std::vector<sprite_t> sprites;
    gatherSprites(sprites, {.mx = mx, .ox = ox, .my = my, .oy = oy});
    for (const auto &sprite : sprites)
    {
        // special case animations
        const int x = sprite.x - mx;
        const int y = sprite.y - my;
        CFrame *tile = specialFrame(sprite.aim, sprite.tileID);
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
            Rect rect{
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
}

void CGameMixin::drawViewPortStatic(CFrame &bitmap)
{
    CMap *map = &m_game->getMap();
    CGame &game = *m_game;

    const int maxRows = HEIGHT / TILE_SIZE;
    const int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());

    const int lmx = std::max(0, game.player().getX() - cols / 2);
    const int lmy = std::max(0, game.player().getY() - rows / 2);
    const int mx = std::min(lmx, map->len() > cols ? map->len() - cols : 0);
    const int my = std::min(lmy, map->hei() > rows ? map->hei() - rows : 0);
    bitmap.fill(BLACK);
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            uint8_t tileID = map->at(x + mx, y + my);
            bool inverted = false;
            std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr;
            CFrame *tile = tile2Frame(tileID, inverted, colorMap);
            if (tile)
                drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);
        }
    }

    std::vector<sprite_t> sprites;
    gatherSprites(sprites, {.mx = mx, .ox = 0, .my = my, .oy = 0});
    for (const auto &sprite : sprites)
    {
        // special case animations
        const int x = sprite.x - mx;
        const int y = sprite.y - my;
        CFrame *tile = specialFrame(sprite.aim, sprite.tileID);
        drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, true);
    }
}

void CGameMixin::centerCamera()
{
    CMap *map = &m_game->getMap();
    CGame &game = *m_game;
    const int maxRows = HEIGHT / TILE_SIZE;
    const int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());
    const int lmx = std::max(0, game.player().getX() - cols / 2);
    const int lmy = std::max(0, game.player().getY() - rows / 2);
    const int mx = std::min(lmx, map->len() > cols ? map->len() - cols : 0);
    const int my = std::min(lmy, map->hei() > rows ? map->hei() - rows : 0);
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
        sprintf(t, "LEVEL %.2d", m_game->level() + 1);
        break;
    case CGame::MODE_RESTART:
        if (m_game->lives() > 1)
        {
            sprintf(t, "LIVES LEFT %.2d", m_game->lives());
        }
        else
        {
            strcpy(t, "LAST LIFE !");
        }
        break;
    case CGame::MODE_GAMEOVER:
        strcpy(t, "GAME OVER");
        break;
    case CGame::MODE_TIMEOUT:
        strcpy(t, "OUT OF TIME");
    };

    const int x = (WIDTH - strlen(t) * 2 * FONT_SIZE) / 2;
    const int y = (HEIGHT - FONT_SIZE * 2) / 2;
    bitmap.fill(BLACK);
    drawFont(bitmap, x, y, t, YELLOW, BLACK, 2, 2);

    if (mode == CGame::MODE_LEVEL_INTRO)
    {
        const char *t = m_game->getMap().title();
        const int x = (WIDTH - strlen(t) * FONT_SIZE) / 2;
        drawFont(bitmap, x, y + 3 * FONT_SIZE, t, WHITE);
    }

    if (mode != CGame::MODE_GAMEOVER)
    {
        const char *hint = m_game->getHintText();
        const int x = (WIDTH - strlen(hint) * FONT_SIZE) / 2;
        const int y = HEIGHT - FONT_SIZE * 4;
        drawFont(bitmap, x, y, hint, CYAN);
    }
    m_currentEvent = EVENT_NONE;
    m_timer = TICK_RATE;
}

void CGameMixin::mainLoop()
{
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
            game.setMode(CGame::MODE_HISCORES);
            if (!m_scoresLoaded)
            {
                m_scoresLoaded = loadScores();
            }
            m_scoreRank = rankScore();
            m_recordScore = m_scoreRank != INVALID;
            m_countdown = HISCORE_DELAY;
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
    case CGame::MODE_PLAY:
        manageGamePlay();
    }
}

void CGameMixin::moveCamera()
{
    CMap &map = CGame::getMap();
    const int maxRows = HEIGHT / TILE_SIZE;
    const int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map.hei());
    const int cols = std::min(maxCols, map.len());

    const int mx = m_cx / 2;
    const int my = m_cy / 2;
    const int x = m_game->player().getX() - mx;
    const int y = m_game->player().getY() - my;

    const int hDeadZone = cols / 2 - 1;
    const int vDeadZone = rows / 2 - 1;

    if (m_cy > 0 && y < vDeadZone)
        --m_cy;
    else if (m_cy < (map.hei() - rows) * 2 &&
             y > rows - vDeadZone)
        ++m_cy;
    else if (m_cx > 0 && x < hDeadZone)
        --m_cx;
    else if (m_cx < (map.len() - cols) * 2 &&
             x > cols - hDeadZone)
        ++m_cx;
}

void CGameMixin::manageGamePlay()
{
    CGame &game = *m_game;
    uint8_t joyState[JOY_AIMS];
    memcpy(joyState, m_joyState, JOY_AIMS);

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

    handleFunctionKeys();
    if (m_paused)
    {
        stopRecorder();
        return;
    }

    manageCurrentEvent();
    manageTimer();
    game.purgeSfx();

    if (m_ticks % game.playerSpeed() == 0 && !game.isPlayerDead())
    {
        game.managePlayer(joyState);
    }

    if (m_ticks % cameraSpeed() == 0)
    {
        moveCamera();
    }

    if (m_ticks % 3 == 0)
    {
        if (game.health() < m_healthRef && m_playerFrameOffset != PLAYER_HIT_FRAME)
        {
            m_playerFrameOffset = PLAYER_HIT_FRAME;
        }
        else if (*(reinterpret_cast<uint32_t *>(joyState)))
        {
            m_playerFrameOffset = (m_playerFrameOffset + 1) % PLAYER_STD_FRAMES;
        }
        else
        {
            m_playerFrameOffset = 0;
        }
        m_healthRef = game.health();
        m_animator->animate();
    }

    game.manageMonsters(m_ticks);

    if (game.isPlayerDead())
    {
        stopRecorder();
        game.killPlayer();

        if (!game.isGameOver())
        {
            restartLevel();
        }
        else
        {
            startCountdown(COUNTDOWN_INTRO);
            game.setMode(CGame::MODE_GAMEOVER);
        }
    }

    if (!game.isGameOver() && game.goalCount() == 0)
    {
        const uint16_t exitKey = m_game->getMap().states().getU(POS_EXIT);
        if (exitKey == 0 || m_game->player().pos() == CMap::toPos(exitKey))
        {
            nextLevel();
        }
    }
}

void CGameMixin::nextLevel()
{
    stopRecorder();
    m_healthRef = 0;
    m_game->nextLevel();
    sanityTest();
    startCountdown(COUNTDOWN_INTRO);
    m_game->loadLevel(CGame::MODE_LEVEL_INTRO);
    openMusicForLevel(m_game->level());
    centerCamera();
}

void CGameMixin::restartLevel()
{
    m_game->restartLevel();
    startCountdown(COUNTDOWN_INTRO);
    centerCamera();
}

void CGameMixin::restartGame()
{
    m_paused = false;
    startCountdown(COUNTDOWN_RESTART);
    m_game->restartGame();
    sanityTest();
    m_game->loadLevel(CGame::MODE_LEVEL_INTRO);
    setupTitleScreen();
}

void CGameMixin::startCountdown(int f)
{
    m_countdown = f * INTRO_DELAY;
}

void CGameMixin::init(CMapArch *maparch, int index)
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
    m_countdown = INTRO_DELAY;
    m_game->loadLevel(CGame::MODE_LEVEL_INTRO);
    centerCamera();
}

void CGameMixin::changeZoom()
{
    setZoom(!m_zoom);
}

void CGameMixin::setZoom(bool zoom)
{
    m_zoom = zoom;
}

bool CGameMixin::within(int val, int min, int max)
{
    return val >= min && val <= max;
}

int CGameMixin::rankScore()
{
    int score = m_game->score();
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
    bitmap.fill(BLACK);
    char t[50];
    int y = 1;
    strcpy(t, "HALL OF HEROES");
    int x = (WIDTH - strlen(t) * 2 * FONT_SIZE) / 2;
    drawFont(bitmap, x, y * FONT_SIZE, t, WHITE, BLACK, 2, 2);
    y += 2;
    strcpy(t, std::string(strlen(t), '=').c_str());
    x = (WIDTH - strlen(t) * 2 * FONT_SIZE) / 2;
    drawFont(bitmap, x, y * FONT_SIZE, t, WHITE, BLACK, 2, 2);
    y += 3;

    for (uint32_t i = 0; i < MAX_SCORES; ++i)
    {
        Color color = i & INTERLINES ? LIGHTGRAY : DARKGRAY;
        if (m_recordScore && m_scoreRank == static_cast<int>(i))
        {
            color = YELLOW;
        }
        bool showCaret = (color == YELLOW) && (m_ticks & CARET_SPEED);
        sprintf(t, " %.8d %.2d %s%c",
                m_hiscores[i].score,
                m_hiscores[i].level,
                m_hiscores[i].name,
                showCaret ? CHARS_CARET : '\0');
        drawFont(bitmap, 1, y * FONT_SIZE, t, color);
        ++y;
    }

    ++y;
    if (m_scoreRank == INVALID)
    {
        strcpy(t, " SORRY, YOU DIDN'T QUALIFY.");
        drawFont(bitmap, 0, y * FONT_SIZE, t, YELLOW);
    }
    else if (m_recordScore)
    {
        strcpy(t, "PLEASE TYPE YOUR NAME AND PRESS ENTER.");
        x = (WIDTH - strlen(t) * FONT_SIZE) / 2;
        drawFont(bitmap, x, y++ * FONT_SIZE, t, YELLOW);
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
            x = (WIDTH - strlen(p) * FONT_SIZE) / 2;
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
        strcat(inputDest, s);
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
}

bool CGameMixin::read(FILE *sfile, std::string &name)
{
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };
    if (!m_game->read(sfile))
    {
        return false;
    }
    clearButtonStates();
    clearJoyStates();
    clearKeyStates();
    m_paused = false;
    m_prompt = PROMPT_NONE;
    readfile(&m_ticks, sizeof(m_ticks));
    readfile(&m_playerFrameOffset, sizeof(m_playerFrameOffset));
    readfile(&m_healthRef, sizeof(m_healthRef));
    readfile(&m_countdown, sizeof(m_countdown));

    size_t ptr = 0;
    fseek(sfile, SAVENAME_PTR_OFFSET, SEEK_SET);
    readfile(&ptr, sizeof(uint32_t));
    fseek(sfile, ptr, SEEK_SET);
    size_t size = 0;
    readfile(&size, sizeof(uint16_t));
    char *tmp = new char[size];
    readfile(tmp, size);
    name = tmp;
    delete[] tmp;
    return true;
}

bool CGameMixin::write(FILE *tfile, const std::string &name)
{
    auto writefile = [tfile](auto ptr, auto size)
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };
    m_game->write(tfile);
    writefile(&m_ticks, sizeof(m_ticks));
    writefile(&m_playerFrameOffset, sizeof(m_playerFrameOffset));
    writefile(&m_healthRef, sizeof(m_healthRef));
    writefile(&m_countdown, sizeof(m_countdown));

    const size_t ptr = ftell(tfile);
    const size_t size = name.size();
    writefile(&size, sizeof(uint16_t));
    writefile(name.c_str(), name.size());
    fseek(tfile, SAVENAME_PTR_OFFSET, SEEK_SET);
    writefile(&ptr, sizeof(uint32_t));
    return true;
}

void CGameMixin::drawPreScreen(CFrame &bitmap)
{
    const char t[] = "CLICK TO START";
    const int x = (WIDTH - strlen(t) * FONT_SIZE) / 2;
    const int y = (HEIGHT - FONT_SIZE) / 2;
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
    for (int y = 0; y < bitmap.hei(); ++y)
    {
        for (int x = 0; x < bitmap.len(); ++x)
        {
            bitmap.at(x, y) =
                ((bitmap.at(x, y) >> bitShift) & colorFilter) | ALPHA;
        }
    }
}

inline uint32_t CGameMixin::fazFilter(int bitShift) const
{
    return (0xff >> bitShift) << 16 |
           (0xff >> bitShift) << 8 |
           0xff >> bitShift;
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
    FILE *tfile = fopen(path.c_str(), "wb");
    if (!tfile)
    {
        fprintf(stderr, "cannot create: %s\n", path.c_str());
        return;
    }
    write(tfile, name);
    fseek(tfile, 0, SEEK_END);
    m_recorder->start(tfile, true);
}

void CGameMixin::playbackGame()
{
    m_recorder->stop();
    std::string name;
    const std::string path = "test.rec";
    FILE *sfile = fopen(path.c_str(), "rb");
    if (!sfile)
    {
        fprintf(stderr, "cannot read: %s\n", path.c_str());
        return;
    }
    read(sfile, name);
    openMusicForLevel(m_game->level());
    m_recorder->start(sfile, false);
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
        Color color = CYAN;
        int scaleX = 1;
        int scaleY = 1;
        int baseY = bitmap.hei() - 4;
        const std::string &s = getEventText(scaleX, scaleY, baseY, color);
        const int x = (WIDTH - s.size() * FONT_SIZE * scaleX) / 2;
        const int y = baseY - FONT_SIZE * scaleY;
        drawFont(bitmap, x, y, s.c_str(), color, CLEAR, scaleX, scaleY);
    }
}

std::string CGameMixin::getEventText(int &scaleX, int &scaleY, int &baseY, Color &color)
{
    if (m_currentEvent == EVENT_SECRET)
    {
        scaleX = 2;
        color = CYAN;
        return "SECRET !";
    }
    else if (m_currentEvent == EVENT_EXTRA_LIFE)
    {
        scaleX = 2;
        color = GREEN;
        return "EXTRA LIFE";
    }
    else if (m_currentEvent == EVENT_SUGAR_RUSH)
    {
        scaleX = 2;
        scaleY = 2;
        if ((m_ticks >> 3) & 1)
            color = LIME;
        else
            color = ORANGE;
        return "SUGAR RUSH";
    }
    else if (m_currentEvent == EVENT_GOD_MODE)
    {
        scaleX = 2;
        color = WHITE;
        return "GOD MODE !";
    }
    else if (m_currentEvent == EVENT_SUGAR)
    {
        scaleX = 2;
        color = PURPLE;
        char tmp[16];
        sprintf(tmp, "YUMMY %d/%d", m_game->sugar(), CGame::SUGAR_RUSH_LEVEL);
        return tmp;
    }
    else if (m_currentEvent >= MSG0)
    {
        scaleX = 1;
        scaleY = 1;
        color = DARKGRAY;
        return m_game->getMap().states().getS(m_currentEvent);
    }
    else
    {
        return "";
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
        m_timer = TICK_RATE;
        CStates &states = game.getMap().states();
        uint16_t timeout = states.getU(TIMEOUT);
        if (timeout == 1)
        {
            game.killPlayer();
            if (game.isGameOver())
            {
                game.setMode(CGame::MODE_GAMEOVER);
            }
            else
            {
                startCountdown(COUNTDOWN_INTRO);
                game.loadLevel(CGame::MODE_TIMEOUT);
                centerCamera();
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

void CGameMixin::drawSugarMeter(CFrame &bitmap, const int bx)
{
    CGame &game = *m_game;
    const int sugar = game.sugar();
    for (int i = 0; i < (int)CGame::SUGAR_RUSH_LEVEL; ++i)
    {
        Rect rect{.x = bx * (int)FONT_SIZE + i * 5, .y = Y_STATUS + 2, .width = 4, .height = 4};
        if (static_cast<int>(i) < sugar)
            drawRect(bitmap, rect, RED, true);
        else
            drawRect(bitmap, rect, WHITE, false);
    }
}

CFrame *CGameMixin::specialFrame(const int aim, const uint8_t tileID)
{
    CFrameSet &animz = *m_animz;
    const AnimzInfo info = m_animator->specialInfo(tileID);
    int saim = tileID < TILES_TOTAL_COUNT ? aim : 0;
    const TileDef &def = getTileDef(tileID);
    if (def.type == TYPE_DRONE)
    {
        saim &= 1;
    }
    return animz[saim * info.frames + info.base + info.offset];
}