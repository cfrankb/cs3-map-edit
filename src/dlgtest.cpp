#include "dlgtest.h"
#include "ui_dlgtest.h"
#include <QShortcut>
#include <QMessageBox>
#include <QPainter>
#include <QKeyEvent>
#include "mapfile.h"
#include "game.h"
#include "tilesdata.h"
#include "Frame.h"
#include "FrameSet.h"
#include "statedata.h"
#include "states.h"
#include "shared/qtgui/qfilewrap.h"
#include "skills.h"

CDlgTest::CDlgTest(QWidget *parent) :
    QDialog(parent),
    CGameMixin(),
    ui(new Ui::CDlgTest)
{
    ui->setupUi(this);
    new QShortcut(QKeySequence(Qt::Key_F11), this, SLOT(changeZoom()));
    m_timer.setInterval(1000 / TICK_RATE);
    m_timer.start();
    setSkill(SKILL_NORMAL);
    m_healthBar = HEALTHBAR_HEARTHS;
}

CDlgTest::~CDlgTest()
{
    delete ui;
}

void CDlgTest::init(CMapArch *mapfile,  const int level)
{
    qDebug("CDlgTest::init");
    setZoom(true);
    CGameMixin::init(mapfile, level);
    //m_cameraMode = CAMERA_MODE_DYNAMIC;
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(mainLoop()));
}

void CDlgTest::exitGame()
{
    reject();
}

void CDlgTest::mainLoop()
{
    CGameMixin::mainLoop();
    update();
}

void CDlgTest::changeZoom()
{
    CGameMixin::changeZoom();
}

void CDlgTest::sanityTest()
{
    CMap *map = m_maparch->at(m_game->level());
    CStates & states = map->states();
    const uint16_t startPos = states.getU(POS_ORIGIN);
    const Pos pos = startPos != 0 ? CMap::toPos(startPos) :  map->findFirst(TILES_ANNIE2);
    QStringList listIssues;

    if ((pos.x == CMap::NOT_FOUND ) && (pos.y == CMap::NOT_FOUND )) {
        listIssues.push_back(tr("No player on map"));
    }
    if (map->count(TILES_DIAMOND) == 0) {
        listIssues.push_back(tr("No diamond on map"));
    }
    if (listIssues.count() > 0) {
        QString msg = tr("Map %1 is incomplete:\n%2").arg(m_game->level() + 1).arg(listIssues.join("\n"));
        QMessageBox::warning(this, "", msg, QMessageBox::Button::Ok);
        exitGame();
    }
}

void CDlgTest::setZoom(bool zoom)
{
    CGameMixin::setZoom(zoom);
    int factor = m_zoom ? 2 : 1;
    this->setMaximumSize(QSize(WIDTH * factor, HEIGHT * factor));
    this->setMinimumSize(QSize(WIDTH * factor, HEIGHT * factor));
    this->resize(QSize(WIDTH * factor, HEIGHT * factor));
}

void CDlgTest::paintEvent(QPaintEvent *)
{
    CFrame bitmap(WIDTH, HEIGHT);
    switch (m_game->mode())
    {
    case CGame::MODE_TIMEOUT:
    case CGame::MODE_LEVEL_INTRO:
    case CGame::MODE_RESTART:
    case CGame::MODE_GAMEOVER:
        drawLevelIntro(bitmap);
        break;
    case CGame::MODE_PLAY:
        drawScreen(bitmap);
        break;
    case CGame::MODE_CLICKSTART:
    case CGame::MODE_HELP:
    case CGame::MODE_IDLE:
    case CGame::MODE_HISCORES:
    case CGame::MODE_TITLE:
    case CGame::MODE_OPTIONS:
    case CGame::MODE_USERSELECT:
    case CGame::MODE_LEVEL_SUMMARY:
        break;
    }

    // show the screen
    const QImage & img = QImage(reinterpret_cast<uint8_t*>(bitmap.getRGB()), bitmap.len(), bitmap.hei(), QImage::Format_RGBX8888);
    const QPixmap & pixmap = QPixmap::fromImage(m_zoom ? img.scaled(QSize(WIDTH * 2, HEIGHT * 2)): img);
    QPainter p(this);
    p.drawPixmap(0, 0, pixmap);
    p.end();
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
        break;
    case Qt::Key_Escape:
        exitGame();
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

void CDlgTest::preloadAssets()
{
    QFileWrap file;


    typedef struct {
        const char *filename;
        CFrameSet **frameset;
    } asset_t;

    asset_t assets[] = {
        {":/data/tiles.obl", &m_tiles},
        {":/data/animz.obl", &m_animz},
        {":/data/annie.obl", &m_users},
    };

    for (int i=0; i < 3; ++i) {
        asset_t & asset = assets[i];
        *(asset.frameset) = new CFrameSet();
        if (file.open(asset.filename, "rb")) {
            qDebug("reading %s", asset.filename);
            if ((*(asset.frameset))->extract(file)) {
                qDebug("extracted: %d", (*(asset.frameset))->getSize());
            }
            file.close();
        }
    }

    const char fontName [] = ":/data/bitfont.bin";
    int size = 0;
    if (file.open(fontName, "rb")) {
        size = file.getSize();
        m_fontData = new uint8_t[size];
        file.read(m_fontData, size);
        file.close();
        qDebug("loaded font: %d bytes", size);
    } else {
        qDebug("failed to open %s", fontName);
    }

    loadColorMaps();
}

void CDlgTest::loadColorMaps()
{
    const char path[] = ":/data/Annie.ini";
    QFileWrap file;
    if (file.open(path, "rb"))
    {
        parseColorMaps(file, m_colormaps);
        file.close();
    }
    else
    {
        qDebug("can't read %s\n", path);
    }
}
