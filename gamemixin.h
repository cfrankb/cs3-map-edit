#ifndef CGAMEMIXIN_H
#define CGAMEMIXIN_H

#ifdef USE_QFILE
#include <QTimer>
#endif
#include <cstdint>

class CFrameSet;
class CGame;
class CFrame;
class CMapArch;
class CAnimator;

class CGameMixin
{
public:
    explicit CGameMixin();
    virtual ~CGameMixin();
    void init(CMapArch *maparch, int index);
    inline bool within(int val, int min, int max);

#ifdef USE_QFILE
protected slots:
#endif
    void mainLoop();
    void changeZoom();

protected:
    enum : uint32_t
    {
        TICK_RATE = 24,
        NO_ANIMZ = 255,
        KEY_PRESSED = 1,
        KEY_RELEASED = 0,
        INTRO_DELAY = TICK_RATE,
        ALPHA = 0xff000000,
        WHITE = 0x00ffffff | ALPHA,
        YELLOW = 0x0000ffff | ALPHA,
        PURPLE = 0x00ff00ff | ALPHA,
        BLACK = 0x00000000 | ALPHA,
        GREEN = 0x0000ff00 | ALPHA,
        LIME = 0x0000ffbf | ALPHA,
        BLUE = 0x00ff0000 | ALPHA,
        DARKBLUE = 0x00440000 | ALPHA,
        DARKGRAY = 0x00444444 | ALPHA,
        LIGHTGRAY = 0x00A9A9A9 | ALPHA,
        WIDTH = 240,
        HEIGHT = 320,
        TILE_SIZE = 16,
        COUNTDOWN_INTRO = 1,
        COUNTDOWN_RESTART = 2,
        FONT_SIZE = 8,
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
    CAnimator *m_animator;
    CFrameSet *m_tiles = nullptr;
    CFrameSet *m_animz = nullptr;
    CFrameSet *m_annie = nullptr;
    uint8_t *m_fontData = nullptr;
    CGame *m_game = nullptr;
    CMapArch *m_maparch = nullptr;
    int m_playerFrameOffset = 0;
    int m_healthRef = 0;
    int m_countdown = 0;
    bool m_zoom = false;
    bool m_assetPreloaded = false;
    void drawScreen(CFrame &bitmap);
    void drawLevelIntro(CFrame &bitmap);
    virtual void preloadAssets();
    inline void drawFont(CFrame &frame, int x, int y, const char *text, const uint32_t color = WHITE);
    inline void drawRect(CFrame &frame, const Rect &rect, const uint32_t color = GREEN, bool fill = true);
    inline void drawKeys(CFrame &bitmap);
    inline void drawTile(CFrame &frame, const int x, const int y, CFrame &tile, bool alpha);
    void nextLevel();
    void restartLevel();
    void restartGame();
    virtual void sanityTest();
    void startCountdown(int f = 1);
    virtual void setZoom(bool zoom);
};

#endif // CGAMEMIXIN_H
