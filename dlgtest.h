#ifndef DLGTEST_H
#define DLGTEST_H

#include <QDialog>
#include <QTimer>
class CFrameSet;
class CMap;
class CGame;
class QKeyEvent;
class CFrame;

namespace Ui {
class CDlgTest;
}

class CDlgTest : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgTest(QWidget *parent = nullptr);
    void init(CMap *map);
    ~CDlgTest();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

protected slots:
    void updateScreen();

private:
    enum {
        TICK_RATE= 24,
        NO_ANIMZ = 255,
        KEY_PRESSED=1,
        KEY_RELEASED=0,
        WHITE  = 0xffffffff,
        YELLOW = 0xff00ffff,
        PURPLE = 0xffff00ff,
        BLACK  = 0xff000000,
        GREEN  = 0xff00ff00,
        LIME   = 0xff34ebb1
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
    QTimer m_timer;
    CGame *m_game;
    void preloadAssets();
    void animate();
    void drawFont(CFrame & frame, int x, int y, const char *text, uint32_t color);
    void drawRect(CFrame & frame, const Rect &rect, uint32_t color = GREEN);
};

#endif // DLGTEST_H
