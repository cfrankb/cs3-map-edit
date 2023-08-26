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

CDlgTest::CDlgTest(QWidget *parent) :
    QDialog(parent),
    CGameMixin(),
    ui(new Ui::CDlgTest)
{
    ui->setupUi(this);
    new QShortcut(QKeySequence(Qt::Key_F11), this, SLOT(changeZoom()));
}

CDlgTest::~CDlgTest()
{
    delete ui;
}

void CDlgTest::init(CMapFile *mapfile)
{
    setZoom(true);
    CGameMixin::init(mapfile, mapfile->currentIndex());
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
    const Pos pos = map->findFirst(TILES_ANNIE2);
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
    case CGame::MODE_INTRO:
    case CGame::MODE_RESTART:
    case CGame::MODE_GAMEOVER:
        drawLevelIntro(bitmap);
        break;
    case CGame::MODE_LEVEL:
        drawScreen(bitmap);
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
