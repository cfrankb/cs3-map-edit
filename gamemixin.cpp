#include "gamemixin.h"
#include "tilesdata.h"
#include "animzdata.h"
#include "FrameSet.h"
#include "Frame.h"
#include "map.h"
#include "game.h"
#include "maparch.h"
#include "shared/qtgui/qfilewrap.h"

typedef struct
{
    uint8_t srcTile;
    uint8_t startSeq;
    uint8_t count;
    uint8_t index;
} AnimzSeq;

AnimzSeq animzSeq[] = {
    {TILES_DIAMOND, ANIMZ_DIAMOND, 13, 0},
    {TILES_INSECT1, ANIMZ_INSECT1, 2, 0},
    {TILES_SWAMP, ANIMZ_SWAMP, 2, 0},
    {TILES_ALPHA, ANIMZ_ALPHA, 2, 0},
    {TILES_FORCEF94, ANIMZ_FORCEF94, 8, 0},
    {TILES_VAMPLANT, ANIMZ_VAMPLANT, 2, 0},
    {TILES_ORB, ANIMZ_ORB, 4, 0},
    {TILES_TEDDY93, ANIMZ_TEDDY93, 2, 0},
    {TILES_LUTIN, ANIMZ_LUTIN, 2, 0},
    {TILES_OCTOPUS, ANIMZ_OCTOPUS, 2, 0},
    {TILES_TRIFORCE, ANIMZ_TRIFORCE, 4, 0},
    {TILES_YAHOO, ANIMZ_YAHOO, 2, 0},
    {TILES_YIGA, ANIMZ_YIGA, 2, 0},
    {TILES_YELKILLER, ANIMZ_YELKILLER, 2, 0},
    {TILES_MANKA, ANIMZ_MANKA, 2, 0},
    // {TILES_MAXKILLER, ANIMZ_MAXKILLER, 1, 0},
    // {TILES_WHTEWORM, ANIMZ_WHTEWORM, 2, 0},
    };

uint8_t tileReplacement[256];

CGameMixin::CGameMixin()
{
    preloadAssets();
    m_game = new CGame();
    memset(tileReplacement, NO_ANIMZ, sizeof(tileReplacement));
    memset(m_joyState, 0, sizeof(m_joyState));
}

CGameMixin::~CGameMixin()
{
    if (m_game) {
        delete m_game;
    }

    if (m_tiles) {
        delete m_tiles;
    }

    if (m_animz) {
        delete m_animz;
    }

    if (m_annie) {
        delete m_annie;
    }

    if (m_fontData) {
        delete [] m_fontData;
    }
}

void CGameMixin::preloadAssets()
{
    QFileWrap file;

    typedef struct {
        const char *filename;
        CFrameSet **frameset;
    } asset_t;

    asset_t assets[] = {
        {":/data/tiles.obl", &m_tiles},
        {":/data/animz.obl", &m_animz},
        {":/data/annie.obl", &m_annie},
    };

    for (int i=0; i < 3; ++i) {
        asset_t & asset = assets[i];
        *(asset.frameset) = new CFrameSet();
        if (file.open(asset.filename, "rb")) {
            qDebug("reading %s", asset.filename);
            if ((*(asset.frameset))->extract(file)) {
                qDebug("exracted: %d", (*(asset.frameset))->getSize());
            }
            file.close();
        }
    }

    const char fontName [] = ":/data/font.bin";
    int size = 0;
    if (file.open(fontName, "rb")) {
        size = file.getSize();
        m_fontData = new uint8_t[size];
        file.read(m_fontData, size);
        file.close();
        qDebug("size: %d", size);
    } else {
        qDebug("failed to open %s", fontName);
    }
}

void CGameMixin::animate()
{
    for (uint i = 0; i < sizeof(animzSeq) / sizeof(AnimzSeq); ++i)
    {
        AnimzSeq &seq = animzSeq[i];
        int j = seq.srcTile;
        tileReplacement[j] = seq.startSeq + seq.index;
        seq.index = seq.index < seq.count - 1 ? seq.index + 1 : 0;
    }
}

void CGameMixin::drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.m_nLen;
    const int fontSize = 8;
    const int fontOffset = fontSize * fontSize;
    const int textSize = strlen(text);
    for (int i=0; i < textSize; ++i) {
        const uint8_t c = static_cast<uint8_t>(text[i]) - ' ';
        uint8_t *font = m_fontData + c * fontOffset;
        for (int yy=0; yy < fontSize; ++yy) {
            for (int xx=0; xx < fontSize; ++xx) {
                rgba[ (yy + y) * rowPixels + xx + x] = *font ? color : BLACK;
                ++font;
            }
        }
        x+= fontSize;
    }
}

void CGameMixin::drawRect(CFrame & frame, const Rect &rect, const uint32_t color, bool fill)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.m_nLen;
    if (fill) {
        for (int y=0; y < rect.height; y++ ) {
            for (int x=0; x < rect.width; x++ ) {
                rgba[ (rect.y + y) * rowPixels + rect.x + x] = color;
            }
        }
    } else {
        for (int y=0; y < rect.height; y++ ) {
            for (int x=0; x < rect.width; x++ ) {
                if (y == 0 || y == rect.height-1 || x ==0 || x == rect.width-1) {
                    rgba[ (rect.y + y) * rowPixels + rect.x + x] = color;
                }
            }
        }
    }
}

void CGameMixin::drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, bool alpha)
{
    const uint32_t *tileData = tile.getRGB();
    uint32_t *dest = bitmap.getRGB() + x + y * WIDTH;
    if (alpha) {
        for (uint32_t row=0; row < TILE_SIZE; ++row) {
            for (uint32_t col=0; col < TILE_SIZE; ++col) {
                const uint32_t & rgba = tileData[col];
                if (rgba & ALPHA) {
                    dest[col] = rgba;
                }
            }
            dest += WIDTH;
            tileData += TILE_SIZE;
        }
    } else {
        for (uint32_t row=0; row < TILE_SIZE; ++row) {
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
    CGame &game = * m_game;
    CFrameSet & tiles = *m_tiles;
    int y = HEIGHT - TILE_SIZE;
    int x = WIDTH - TILE_SIZE;
    const uint8_t *keys = game.keys();
    for (int i=0; i < 6; ++i) {
        uint8_t k = keys[i];
        if (k) {
            drawTile(bitmap, x,y, * tiles[k], true);
            x -= TILE_SIZE;
        }
    }
}

void CGameMixin::drawScreen(CFrame & bitmap) {
    CMap *map = & m_game->getMap();
    CGame &game = * m_game;

    int maxRows = HEIGHT / TILE_SIZE;
    int maxCols = WIDTH / TILE_SIZE;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());

    const int lmx = std::max(0, game.player().getX() - cols / 2);
    const int lmy = std::max(0, game.player().getY() - rows / 2);
    const int mx = std::min(lmx, map->len() > cols ? map->len() - cols : 0);
    const int my = std::min(lmy, map->hei() > rows ? map->hei() - rows : 0);

    CFrameSet & tiles = *m_tiles;
    CFrameSet & animz = *m_animz;
    CFrameSet & annie = *m_annie;
    bitmap.fill(BLACK);
    for (int y=0; y < rows; ++y) {
        if (y + my >= map->hei())
        {
            break;
        }
        for (int x=0; x < cols; ++x) {
            uint8_t tileID = map->at(x + mx, y + my);
            CFrame *tile;
            if (tileID == TILES_ANNIE2)
            {
                tile = annie[game.player().getAim() * 4 + m_playerFrameOffset];
            }
            else
            {
                if (tileID == TILES_STOP || tileID == TILES_BLANK)
                {
                    continue;
                }
                int j = tileReplacement[tileID];
                if (j == NO_ANIMZ) {
                    tile = tiles[tileID];
                } else {
                    tile = animz[j];
                }
            }
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
    drawFont(bitmap, bx * 8, 2, tmp, YELLOW);
    bx += strlen(tmp);
    sprintf(tmp, "LIVES %.2d ", game.lives());
    drawFont(bitmap, bx * 8, 2, tmp, PURPLE);

    // draw bottom rect
    drawRect(bitmap, Rect{0, bitmap.m_nHei - 16, WIDTH, TILE_SIZE}, LIGHTSLATEGRAY, true);
    drawRect(bitmap, Rect{0, bitmap.m_nHei - 16, WIDTH, TILE_SIZE}, LIGHTGRAY, false);

    // draw health bar
    drawRect(bitmap, Rect{4, bitmap.m_nHei - 12, std::min(game.health() / 2, bitmap.m_nLen - 4), 8}, LIME, true);
    drawRect(bitmap, Rect{4, bitmap.m_nHei - 12, std::min(game.health() / 2, bitmap.m_nLen - 4), 8}, WHITE, false);

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

    int x = (WIDTH - strlen(t) * 8) / 2;
    int y = (HEIGHT - 8) / 2;
    bitmap.fill(BLACK);
    drawFont(bitmap, x, y, t, WHITE);
}

void CGameMixin::mainLoop()
{
    CGame &game = * m_game;
    if (m_countdown > 0) {
        --m_countdown;
    }

    switch (game.mode())
    {
    case CGame::MODE_INTRO:
    case CGame::MODE_RESTART:
    case CGame::MODE_GAMEOVER:
        if (m_countdown) {
            return;
        }
        if (game.mode()== CGame::MODE_GAMEOVER) {
            restartGame();
        } else {
            game.setMode(CGame::MODE_LEVEL);
        }
        break;
    }

    if (m_ticks % 3 == 0 && !game.isPlayerDead())
    {
        game.managePlayer(m_joyState);
    }

    if (m_ticks % 3 == 0)
    {
        if (game.health() < m_healthRef && m_playerFrameOffset != 3) {
            m_playerFrameOffset = 3;
        } else if (*(reinterpret_cast<uint32_t*>(m_joyState))){
            m_playerFrameOffset = (m_playerFrameOffset + 1) % 3;
        } else {
            m_playerFrameOffset = 0;
        }
        m_healthRef = game.health();
        animate();
    }

    if ((m_ticks & 3) == 0)
    {
        game.manageMonsters();
    }

    if (game.isPlayerDead()){
        game.killPlayer();

        if(!game.isGameOver()) {
            restartLevel();
        } else {
            startCountdown(COUNTDOWN_INTRO);
            game.setMode(CGame::MODE_GAMEOVER);
        }
    }

    ++m_ticks;

    if (!game.isGameOver()) {
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
    m_maparch = maparch;
    m_game->setMapArch(maparch);
    m_game->setLevel(index);
    sanityTest();

    m_countdown = INTRO_DELAY;
    m_game->loadLevel(false);

    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
}

void CGameMixin::changeZoom()
{
    setZoom(!m_zoom);
}

void CGameMixin::setZoom(bool zoom)
{
    m_zoom = zoom;
}

void CGameMixin::sanityTest()
{
    qDebug("TODO: sanityTest() to be implemented in child class");
}