#pragma once

#include <QTimer>
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
    void highlight(uint8_t attr);
    void highlight(uint8_t x, uint8_t y);

protected:
    virtual void paintEvent(QPaintEvent *) ;
    enum:int32_t {
        FONT_SIZE = 8,
        NO_ANIMZ = 255,
        TICK_RATE = 24,
        TILE_SIZE = 16,
        ANIMATION_RATE =3,
        SCALE_FACTOR=2,
        BLINK_RATE=4,
    };

    struct Rect
    {
        int x;
        int y;
        int width;
        int height;
    };

    void preloadAssets();
    void drawScreen(CFrame &bitmap);
    void drawFont(CFrame & frame, int x, int y, const char *text, const uint32_t color, const bool alpha);
    void drawTile(CFrame & bitmap, const int x, const int y, CFrame & tile, const bool alpha);
    void drawRect(CFrame &frame, const Rect &rect, const uint32_t color, bool fill);
    uint32_t attr2color(const uint8_t attr);

    QTimer m_timer;
    std::unique_ptr<CFrameSet> m_tiles;
    std::unique_ptr<CFrameSet> m_animz;
    std::vector<uint8_t> m_fontData;
    CMap *m_map = nullptr;
    std::unique_ptr<CAnimator> m_animator;
    bool m_showGrid = false;
    bool m_animate = false;
    uint64_t m_ticks = 0;
    friend class CMapScroll;
    uint8_t m_attr = 0;
    uint8_t m_hx = 0;
    uint8_t m_hy = 0;
    std::unique_ptr<CFrame> m_frame;
};
