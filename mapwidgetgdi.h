#ifndef CMAPWIDGETGDI_H
#define CMAPWIDGETGDI_H

#include "qtimer.h"
#include <QWidget>
class CMap;
class CFrame;
class CFrameSet;

class CMapWidgetGDI : public QWidget
{
    Q_OBJECT
public:
    explicit CMapWidgetGDI(QWidget *parent = nullptr);
    virtual ~CMapWidgetGDI();
    void setMap(CMap *pMap);

signals:

protected slots:
    void showGrid(bool show);

protected:
    virtual void paintEvent(QPaintEvent *) ;
    enum:uint32_t {
        TICK_RATE = 20,
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
    void drawScreen(CFrame &bitmap);
    void drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color);
    void drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, bool alpha);
    void drawGrid(CFrame & bitmap);

    QTimer m_timer;
    CFrameSet *m_tiles = nullptr;
    uint8_t *m_fontData = nullptr;
    CMap *m_map = nullptr;
    bool m_showGrid = false;
    friend class CMapScroll;
};

#endif // CMAPWIDGETGDI_H
