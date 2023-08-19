#ifndef CMAPWIDGETGDI_H
#define CMAPWIDGETGDI_H

#include "qtimer.h"
#include <QWidget>
class CMap;
class CFrame;
class CFrameSet;
class CAnimator;

class CMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CMapWidget(QWidget *parent = nullptr);
    virtual ~CMapWidget();
    void setMap(CMap *pMap);

signals:

protected slots:
    void showGrid(bool show);
    void setAnimate(bool val);

protected:
    virtual void paintEvent(QPaintEvent *) ;
    enum:uint32_t {
        FONT_SIZE = 8,
        NO_ANIMZ = 255,
        TICK_RATE = 24,
        TILE_SIZE = 16,
        ALPHA  = 0xff000000,
        WHITE  = 0x00ffffff | ALPHA,
        YELLOW = 0x0000ffff | ALPHA,
        PURPLE = 0x00ff00ff | ALPHA,
        BLACK  = 0x00000000 | ALPHA,
        GREEN  = 0x0000ff00 | ALPHA,
        LIME   = 0x0034ebb1 | ALPHA,
        BLUE   = 0x00ff0000 | ALPHA,
        DARKBLUE = 0x00440000 | ALPHA,
        LIGHTSLATEGRAY= 0x00998877 | ALPHA,
        LIGHTGRAY= 0x00DCDCDC | ALPHA,
        GRIDCOLOR = 0x00bfa079 | ALPHA
    };

    void preloadAssets();
    inline void drawScreen(CFrame &bitmap);
    inline void drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color, const bool alpha);
    inline void drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, const bool alpha);
    inline void drawGrid(CFrame & bitmap);

    QTimer m_timer;
    CFrameSet *m_tiles = nullptr;
    CFrameSet *m_animz = nullptr;
    uint8_t *m_fontData = nullptr;
    CMap *m_map = nullptr;
    CAnimator *m_animator = nullptr;
    bool m_showGrid = false;
    bool m_animate = false;
    uint32_t m_ticks = 0;
    friend class CMapScroll;
};

#endif // CMAPWIDGETGDI_H
