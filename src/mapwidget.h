#ifndef CMAPWIDGETGDI_H
#define CMAPWIDGETGDI_H

#include "qtimer.h"
#include <QWidget>
class CMap;
class CFrame;
class CFrameSet;
class CAnimator;

#define RGBA(R, G, B) (R | (G << 8) | (B << 16) | 0xff000000)

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
    enum:int32_t {
        FONT_SIZE = 8,
        NO_ANIMZ = 255,
        TICK_RATE = 24,
        TILE_SIZE = 16,
    };
    enum:uint32_t {
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
        GRIDCOLOR = 0x00bfa079 | ALPHA,
        RED    = 0x0000ff | ALPHA,
        CYAN   = 0xffff00 | ALPHA,
        PINK = RGBA(0xff, 0xc0, 0xcb),      // #ffc0cb
        HOTPINK = RGBA(0xff, 0x69, 0xb4),   // #ff69b4
        DEEPPINK = RGBA(0xff, 0x14, 0x93),  // #ff1493
        ORANGE = RGBA(0xff, 0xaf,0x00),     // #ffaf00
        DARKORANGE = RGBA(0xff,0x8c,0x00),  // #ff8c00
        CORAL = RGBA(0xff, 0x7f, 0x50),     // #ff7f50
        OLIVE = RGBA(0x80, 0x80, 0x00),     // #808000
        MEDIUMSEAGREEN= RGBA(0x3C,0xB3, 0x71), // #3CB371
        SEAGREEN = RGBA(0x2E, 0x8B, 0x57),  // #2E8B57
        BLUEVIOLET = RGBA(0x8A, 0x2B, 0xE2),// #8A2BE2
        DEEPSKYBLUE = RGBA(0x00, 0xBF, 0xFF), // #00BFFF
    };

    using Rect = struct
    {
        int x;
        int y;
        int width;
        int height;
    };

    void preloadAssets();
    inline void drawScreen(CFrame &bitmap);
    inline void drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color, const bool alpha);
    inline void drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, const bool alpha);
    inline void drawGrid(CFrame & bitmap);
    void drawRect(CFrame &frame, const Rect &rect, const uint32_t color, bool fill);
    uint32_t attr2color(const uint8_t attr);

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
