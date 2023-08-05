#ifndef DLGTEST_H
#define DLGTEST_H

#include <QDialog>
#include <QTimer>
class CFrameSet;
class CMap;
class CMapFile;
class CGame;
class QKeyEvent;
class CFrame;
class QPaintEvent;

namespace Ui {
class CDlgTest;
}

class CDlgTest : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgTest(QWidget *parent = nullptr);
    void init(CMapFile *mapfile);
    ~CDlgTest();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *) override;

protected slots:
    void mainLoop();
    void changeZoom();

private:
    enum {
        TICK_RATE= 24,
        NO_ANIMZ = 255,
        KEY_PRESSED=1,
        KEY_RELEASED=0,
        INTRO_DELAY = TICK_RATE,
        ALPHA  = 0xff000000,
        WHITE  = 0x00ffffff | ALPHA,
        YELLOW = 0x0000ffff | ALPHA,
        PURPLE = 0x00ff00ff | ALPHA,
        BLACK  = 0x00000000 | ALPHA,
        GREEN  = 0x0000ff00 | ALPHA,
        LIME   = 0x0034ebb1 | ALPHA,
        WIDTH  = 240,
        HEIGHT = 320,
    };

    typedef struct
    {
        int x;
        int y;
        int width;
        int height;
    } Rect;

    uint8_t m_joyState[4];
    uint32_t m_ticks = 0;
    Ui::CDlgTest *ui;
    CFrameSet *m_tiles = nullptr;
    CFrameSet *m_animz = nullptr;
    CFrameSet *m_annie = nullptr;
    uint8_t *m_fontData = nullptr;
    CGame *m_game = nullptr;
    CMapFile * m_mapfile = nullptr;
    QTimer m_timer;
    int m_countdown = 0;
    bool m_zoom = false;
    void drawScreen(CFrame &bitmap);
    void drawLevelIntro(CFrame &bitmap);
    void preloadAssets();
    void animate();
    void drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color = WHITE);
    void drawRect(CFrame & frame, const Rect &rect, const uint32_t color = GREEN);
    void nextLevel();
    void restartLevel();
    void restartGame();
    void sanityTest();
    void startCountdown();
    void setZoom(bool zoom);
};

#endif // DLGTEST_H
