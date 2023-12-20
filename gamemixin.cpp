#include "gamemixin.h"
#include "tilesdata.h"
#include "animzdata.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"
#include "map.h"
#include "game.h"
#include "maparch.h"
#include "animator.h"

CGameMixin::CGameMixin()
{
    m_game = new CGame();
    m_animator = new CAnimator();
    memset(m_joyState, 0, sizeof(m_joyState));
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
}

void CGameMixin::preloadAssets()
{
    // TODO: implement in child class
}

void CGameMixin::drawFont(CFrame &frame, int x, int y, const char *text, const uint32_t color)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.len();
    const int fontSize = 8;
    const int fontOffset = fontSize;
    const int textSize = strlen(text);
    for (int i = 0; i < textSize; ++i)
    {
        const uint8_t c = static_cast<uint8_t>(text[i]) - ' ';
        uint8_t *font = m_fontData + c * fontOffset;
        for (int yy = 0; yy < fontSize; ++yy)
        {
            uint8_t bitFilter = 1;
            for (int xx = 0; xx < fontSize; ++xx)
            {
                rgba[(yy + y) * rowPixels + xx + x] = font[yy] & bitFilter ? color : BLACK;
                bitFilter = bitFilter << 1;
            }
        }
        x += fontSize;
    }
}

void CGameMixin::drawRect(CFrame &frame, const Rect &rect, const uint32_t color, bool fill)
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

void CGameMixin::drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, bool alpha)
{
    const uint32_t *tileData = tile.getRGB();
    uint32_t *dest = bitmap.getRGB() + x + y * WIDTH;
    if (alpha)
    {
        for (uint32_t row = 0; row < TILE_SIZE; ++row)
        {
            for (uint32_t col = 0; col < TILE_SIZE; ++col)
            {
                const uint32_t &rgba = tileData[col];
                if (rgba & ALPHA)
                {
                    dest[col] = rgba;
                }
            }
            dest += WIDTH;
            tileData += TILE_SIZE;
        }
    }
    else
    {
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
            dest += WIDTH;
        }
    }
}

void CGameMixin::drawKeys(CFrame &bitmap)
{
    CGame &game = *m_game;
    CFrameSet &tiles = *m_tiles;
    int y = HEIGHT - TILE_SIZE;
    int x = WIDTH - TILE_SIZE;
    const uint8_t *keys = game.keys();
    for (int i = 0; i < 6; ++i)
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

    CFrameSet &tiles = *m_tiles;
    CFrameSet &animz = *m_animz;
    CFrameSet &annie = *m_annie;
    bitmap.fill(BLACK);
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            uint8_t tileID = map->at(x + mx, y + my);
            CFrame *tile;
            if (tileID == TILES_STOP || tileID == TILES_BLANK || m_animator->isSpecialCase(tileID))
            {
                // skip blank tiles and special cases
                continue;
            }
            else if (tileID == TILES_ANNIE2)
            {
                tile = annie[game.player().getAim() * 8 + m_playerFrameOffset];
            }
            else
            {
                int j = m_animator->at(tileID);
                if (j == NO_ANIMZ)
                {
                    tile = tiles[tileID];
                }
                else
                {
                    tile = animz[j];
                }
            }
            drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);
        }
    }

    const int offset = m_animator->offset() & 7;
    CActor *monsters;
    int count;
    game.getMonsters(monsters, count);
    for (int i = 0; i < count; ++i)
    {
        const CActor &monster = monsters[i];
        if (monster.within(mx, my, mx + cols, my + rows))
        {
            const uint8_t tileID = map->at(monster.getX(), monster.getY());
            if (!m_animator->isSpecialCase(tileID))
            {
                continue;
            }
            // special case animations
            const int x = monster.getX() - mx;
            const int y = monster.getY() - my;
            CFrame *tile = animz[monster.getAim() * 8 + ANIMZ_INSECT1 + offset];
            drawTile(bitmap, x * TILE_SIZE, y * TILE_SIZE, *tile, false);
        }
    }

    // draw game status
    char tmp[32];
    int bx = 0;
    sprintf(tmp, "%.8d ", game.score());
    drawFont(bitmap, 0, 2, tmp, WHITE);
    bx += strlen(tmp);
    sprintf(tmp, "DIAMONDS %.2d ", game.diamonds());
    drawFont(bitmap, bx * FONT_SIZE, 2, tmp, YELLOW);
    bx += strlen(tmp);
    sprintf(tmp, "LIVES %.2d ", game.lives());
    drawFont(bitmap, bx * FONT_SIZE, 2, tmp, PURPLE);

    // draw bottom rect
    drawRect(bitmap, Rect{0, bitmap.hei() - 16, WIDTH, TILE_SIZE}, DARKGRAY, true);
    drawRect(bitmap, Rect{0, bitmap.hei() - 16, WIDTH, TILE_SIZE}, LIGHTGRAY, false);

    // draw health bar
    drawRect(bitmap, Rect{4, bitmap.hei() - 12, std::min(game.health() / 2, bitmap.len() - 4), 8},
             game.godModeTimer() ? WHITE : LIME, true);
    drawRect(bitmap, Rect{4, bitmap.hei() - 12, std::min(game.health() / 2, bitmap.len() - 4), 8},
             WHITE, false);

    drawKeys(bitmap);
}

void CGameMixin::drawLevelIntro(CFrame &bitmap)
{
    char t[32];
    switch (m_game->mode())
    {
    case CGame::MODE_INTRO:
        sprintf(t, "LEVEL %.2d", m_game->level() + 1);
        break;
    case CGame::MODE_RESTART:
        sprintf(t, "LIVES LEFT %.2d", m_game->lives());
        break;
    case CGame::MODE_GAMEOVER:
        strcpy(t, "GAME OVER");
    };

    int x = (WIDTH - strlen(t) * FONT_SIZE) / 2;
    int y = (HEIGHT - FONT_SIZE) / 2;
    bitmap.fill(BLACK);
    drawFont(bitmap, x, y, t, WHITE);
}

void CGameMixin::mainLoop()
{
    CGame &game = *m_game;
    if (m_countdown > 0)
    {
        --m_countdown;
    }

    switch (game.mode())
    {
    case CGame::MODE_INTRO:
    case CGame::MODE_RESTART:
    case CGame::MODE_GAMEOVER:
        if (m_countdown)
        {
            return;
        }
        if (game.mode() == CGame::MODE_GAMEOVER)
        {
            restartGame();
        }
        else
        {
            game.setMode(CGame::MODE_LEVEL);
        }
        break;
    }

    if (m_ticks % game.playerSpeed() == 0 && !game.isPlayerDead())
    {
        game.managePlayer(m_joyState);
    }

    if (m_ticks % 3 == 0)
    {
        if (game.health() < m_healthRef && m_playerFrameOffset != 7)
        {
            m_playerFrameOffset = 7;
        }
        else if (*(reinterpret_cast<uint32_t *>(m_joyState)))
        {
            m_playerFrameOffset = (m_playerFrameOffset + 1) % 7;
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

    ++m_ticks;

    if (!game.isGameOver())
    {
        if (game.goalCount() == 0)
        {
            nextLevel();
        }
    }
}

void CGameMixin::nextLevel()
{
    m_healthRef = 0;
    m_game->nextLevel();
    sanityTest();
    startCountdown(COUNTDOWN_INTRO);
    m_game->loadLevel(false);
}

void CGameMixin::restartLevel()
{
    startCountdown(COUNTDOWN_INTRO);
    m_game->loadLevel(true);
}

void CGameMixin::restartGame()
{
    startCountdown(COUNTDOWN_RESTART);
    m_game->restartGame();
    sanityTest();
    m_game->loadLevel(false);
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
    m_game->loadLevel(false);
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

void CGameMixin::sanityTest()
{
    // TODO: sanityTest() to be implemented in child class
}
