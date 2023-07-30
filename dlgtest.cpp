#include "dlgtest.h"
#include "tilesdata.h"
#include "ui_dlgtest.h"
#include "animzdata.h"
#include "FrameSet.h"
#include "Frame.h"
#include "map.h"
#include "game.h"
#include "shared/qtgui/qfilewrap.h"
#include "shared/qtgui/qthelper.h"
#include <QKeyEvent>

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

CDlgTest::CDlgTest(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgTest)
{
    preloadAssets();
    m_game = new CGame();
    ui->setupUi(this);
    memset(tileReplacement, NO_ANIMZ, sizeof(tileReplacement));
    memset(m_joyState, 0, sizeof(m_joyState));
}

CDlgTest::~CDlgTest()
{
    delete ui;
    delete m_game;
    delete m_tiles;
    delete m_animz;
    delete m_annie;
    delete [] m_fontData;
}

void CDlgTest::preloadAssets()
{
    QFileWrap file;
    m_tiles = new CFrameSet();
    if (file.open(":/data/tiles.obl", "rb")) {
        qDebug("reading tiles");
        if (m_tiles->extract(file)) {
            qDebug("exracted: %d", m_tiles->getSize());
        }
        file.close();
    }

    m_animz = new CFrameSet();
    if (file.open(":/data/animz.obl", "rb")) {
        qDebug("reading animz");
        if (m_animz->extract(file)) {
            qDebug("exracted: %d", m_animz->getSize());
        }
        file.close();
    }

    m_annie = new CFrameSet();
    if (file.open(":/data/annie.obl", "rb")) {
        qDebug("reading annie");
        if (m_annie->extract(file)) {
            qDebug("exracted: %d", m_annie->getSize());
        }
        file.close();
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

void CDlgTest::animate()
{
    for (uint i = 0; i < sizeof(animzSeq) / sizeof(AnimzSeq); ++i)
    {
        AnimzSeq &seq = animzSeq[i];
        int j = seq.srcTile;
        tileReplacement[j] = seq.startSeq + seq.index;
        seq.index = seq.index < seq.count - 1 ? seq.index + 1 : 0;
    }
}

void CDlgTest::drawFont(CFrame & frame, int x, int y, const char *text, uint32_t color)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.m_nLen;
    const int fontSize = 8;
    for (int i=0; text[i]; ++i) {
        const uint8_t c = text[i] - ' ';
        uint8_t *font = m_fontData + c * fontSize * fontSize;
        for (int yy=0; yy < fontSize; ++yy) {
            for (int xx=0; xx < fontSize; ++xx) {
                rgba[ (yy + y) * rowPixels + xx + x] = *font ? color : BLACK;
                ++font;
            }
        }
        x+= 8;
    }
}

void CDlgTest::drawRect(CFrame & frame, const Rect &rect, uint32_t color)
{
    uint32_t *rgba = frame.getRGB();
    const int rowPixels = frame.m_nLen;
    for (int y=0; y < rect.height; y++ ) {
        for (int x=0; x < rect.width; x++ ) {
            rgba[ (rect.y + y) * rowPixels + rect.x + x] = color;
        }
    }
}

void CDlgTest::updateScreen()
{
    CMap *map = & m_game->getMap();
    CGame &game = * m_game;
    QSize size = ui->sMapView->size();
    const int maxRows = size.height() / 16;
    const int maxCols = size.width() / 16;
    const int rows = std::min(maxRows, map->hei());
    const int cols = std::min(maxCols, map->len());

    const int lmx = std::max(0, game.player().getX() - cols / 2);
    const int lmy = std::max(0, game.player().getY() - rows / 2);
    const int mx = std::min(lmx, map->len() > cols ? map->len() - cols : 0);
    const int my = std::min(lmy, map->hei() > rows ? map->hei() - rows : 0);

    const int tileSize = 16;
    const int lineSize = maxCols * tileSize;
    CFrameSet & tiles = *m_tiles;
    CFrameSet & animz = *m_animz;
    CFrameSet & annie = *m_annie;
    CFrame bitmap(maxCols * tileSize, maxRows *tileSize);
    bitmap.fill(0xff000000);
    uint32_t *rgba = bitmap.getRGB();
    for (int row=0; row < rows; ++row) {
        if (row + my >= map->hei())
        {
            break;
        }
        for (int col=0; col < cols; ++col) {
            uint8_t tileID = map->at(col + mx, row + my);
            CFrame *tile;

            if (tileID == TILES_ANNIE2)
            {
                tile = annie[game.player().getAim() * 4 + col % 3];
            }
            else
            {
                if (tileID == TILES_STOP)
                {
                    tileID = TILES_BLANK;
                }
                int j = tileReplacement[tileID];
                if (j == NO_ANIMZ) {
                    tile = tiles[tileID];
                } else {
                    tile = animz[j];
                }
            }
            for (int y=0; y < tileSize; ++y) {
                for (int x=0; x < tileSize; ++x) {
                    rgba[x + col*tileSize+ y * lineSize + row * tileSize*lineSize] = tile->at(x,y) | 0xff000000;
                }
            }
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

    // draw health bar
    drawRect(bitmap, Rect{.x = 4, .y = bitmap.m_nHei - 10, .width = std::min(game.health() / 2, static_cast<int>(bitmap.m_nLen) - 4), .height = 8}, LIME);

    // bitmap.shrink();
    QPixmap pixmap = frame2pixmap(bitmap);
    ui->sMapView->setPixmap(pixmap);

    if (m_ticks % 3 == 0 && !game.isPlayerDead())
    {
        game.managePlayer(m_joyState);
    }

    if (m_ticks % 3 == 0)
    {
        animate();
    }

    if (m_ticks % 4 == 0)
    {
        game.manageMonsters();
    }

    if (game.isPlayerDead()){
        game.killPlayer();
        /*
        //sleep_ms(500);
        if(!game.isGameOver()) {
            game.restartLevel();
        } else {
            game.setMode(CGame::MODE_GAMEOVER);
        }*/
    }

    ++m_ticks;

    /*
    //uint16_t joy =engine->readJoystick();
    if (!game.isGameOver()) {
        if (game.goalCount() == 0 || joy & JOY_A_BUTTON)
        {
            game.nextLevel();
        }
    } else {
        if (joy & JOY_BUTTON) {
        }
    }
    */
}


void CDlgTest::init(CMap *map)
{
    m_game->getMap() = *map;
    m_game->loadLevel();

    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateScreen()));
}

void CDlgTest::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Up:
        m_joyState[AIM_UP] = KEY_PRESSED;
        break;
    case Qt::Key_Down:
        m_joyState[AIM_DOWN] = KEY_PRESSED;
        break;
    case Qt::Key_Left:
        m_joyState[AIM_LEFT] = KEY_PRESSED;
        break;
    case Qt::Key_Right:
        m_joyState[AIM_RIGHT] = KEY_PRESSED;
    }
}

void CDlgTest::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Up:
        m_joyState[AIM_UP] = KEY_RELEASED;
        break;
    case Qt::Key_Down:
        m_joyState[AIM_DOWN] = KEY_RELEASED;
        break;
    case Qt::Key_Left:
        m_joyState[AIM_LEFT] = KEY_RELEASED;
        break;
    case Qt::Key_Right:
        m_joyState[AIM_RIGHT] = KEY_RELEASED;
    }
}

