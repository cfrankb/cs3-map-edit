/*
    cs3-runtime-sdl
    Copyright (C) 2024  Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef USE_QFILE
#include <QTimer>
#endif
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>

#define WIDTH getWidth()
#define HEIGHT getHeight()

class CActor;
class CFrameSet;
class CGame;
class CFrame;
class CMapArch;
class CAnimator;
class IMusic;
class CRecorder;

#define RGBA(R, G, B) (R | (G << 8) | (B << 16) | 0xff000000)

class CGameMixin
{
public:
    explicit CGameMixin();
    virtual ~CGameMixin();
    virtual void init(CMapArch *maparch, const int index);
    inline bool isWithin(int val, int min, int max);
    void enableHiScore();
    void setSkill(uint8_t skill);
    static int tickRate();
    void setWidth(int w);
    void setHeight(int h);

#ifdef USE_QFILE
protected slots:
#endif
    virtual void mainLoop();
    void changeZoom();
    virtual void save() = 0;
    virtual void load() = 0;

protected:
    enum : uint32_t
    {
        TICK_RATE = 24,
        NO_ANIMZ = 255,
        KEY_PRESSED = 1,
        KEY_RELEASED = 0,
        BUTTON_PRESSED = 1,
        BUTTON_RELEASED = 0,
        INTRO_DELAY = TICK_RATE * 3,
        HISCORE_DELAY = 5 * TICK_RATE,
        EVENT_COUNTDOWN_DELAY = TICK_RATE,
        MSG_COUNTDOWN_DELAY = 3 * TICK_RATE,
        TILE_SIZE = 16,
        COUNTDOWN_INTRO = 1,
        COUNTDOWN_RESTART = 2,
        GAME_MENU_COOLDOWN = 10,
        FONT_SIZE = 8,
        MAX_SCORES = 18,
        KEY_REPETE_DELAY = 5,
        KEY_NO_REPETE = 1,
        MAX_NAME_LENGTH = 16,
        SAVENAME_PTR_OFFSET = 8,
        CARET_SPEED = 8,
        INTERLINES = 2,
        Y_STATUS = 2,
        PLAYER_FRAMES = 8,
        PLAYER_HIT_FRAME = 7,
        PLAYER_STD_FRAMES = 7,
        ANIMZ_INSECT1_FRAMES = 8,
        INSECT1_MAX_OFFSET = 7,
        CAMERA_MODE_STATIC = 0,
        CAMERA_MODE_DYNAMIC = 1,
        FAZ_INV_BITSHIFT = 1,
        INDEX_ANNIE_DEAD = 4,
    };

    enum Color : uint32_t
    {
        CLEAR = 0,
        ALPHA = 0xff000000,
        WHITE = RGBA(0xff, 0xff, 0xff),      // #ffffff
        YELLOW = RGBA(0xff, 0xff, 0x00),     // #ffff00
        PURPLE = RGBA(0xff, 0x00, 0xff),     // #ff00ff
        DARKPURPLE = RGBA(0x4a, 0x45, 0x98), // #4a4598
        BLACK = RGBA(0x00, 0x00, 0x00),      // #000000
        GREEN = RGBA(0x00, 0xff, 0x00),      // #00ff00
        DARKGREEN = RGBA(0x00, 0x80, 0x00),  // #008000
        LIME = RGBA(0xbf, 0xff, 0x00),       // #bfff00
        BLUE = RGBA(0x00, 0x00, 0xff),       // #0000ff
        MIDBLUE = RGBA(0x00, 0x00, 0x80),    // #000080
        CYAN = RGBA(0x00, 0xff, 0xff),       // #00ffff
        RED = RGBA(0xff, 0x00, 0x00),        // #ff0000
        DARKRED = RGBA(0x80, 0x00, 0x00),    // #800000
        DARKBLUE = RGBA(0x00, 0x00, 0x44),   // #000044
        DARKGRAY = RGBA(0x44, 0x44, 0x44),   // #444444
        GRAY = RGBA(0x88, 0x88, 0x88),       // #808080
        LIGHTGRAY = RGBA(0xa9, 0xa9, 0xa9),  // #a9a9a9
        ORANGE = RGBA(0xf5, 0x9b, 0x14),     // #f59b14
        PINK = RGBA(0xe0, 0xa8, 0xe0),       // #e0a8e0
    };

    enum KeyCode : uint8_t
    {
        Key_A,
        Key_N = Key_A + 13,
        Key_Y = Key_A + 24,
        Key_Z = Key_A + 25,
        Key_Space,
        Key_0,
        Key_9 = Key_0 + 9,
        Key_Period,
        Key_BackSpace,
        Key_Enter,
        Key_Escape,
        Key_F1,
        Key_F2,
        Key_F3,
        Key_F4,
        Key_F5,
        Key_F6,
        Key_F7,
        Key_F8,
        Key_F9,
        Key_F10,
        Key_F11,
        Key_F12,
        Key_Count
    };

    enum Prompt
    {
        PROMPT_NONE,
        PROMPT_ERASE_SCORES,
        PROMPT_RESTART_GAME,
        PROMPT_LOAD,
        PROMPT_SAVE,
        PROMPT_RESTART_LEVEL,
        PROMPT_HARDCORE,
        PROMPT_TOGGLE_MUSIC,
    };

    enum
    {
        INVALID = -1,
        JOY_AIMS = 4,
    };

    enum Button
    {
        BUTTON_A = 0,
        BUTTON_B,
        BUTTON_X,
        BUTTON_Y,
        BUTTON_START,
        BUTTON_BACK,
        Button_Count,
    };

    uint8_t m_keyStates[Key_Count];
    uint8_t m_keyRepeters[Key_Count];

    struct Rect
    {
        int x;
        int y;
        int width;
        int height;
    };

    struct cameraContext_t
    {
        int mx;
        int ox;
        int my;
        int oy;
    };

    struct sprite_t
    {
        uint16_t x;
        uint16_t y;
        uint8_t tileID;
        uint8_t aim;
    };

    struct hiscore_t
    {
        int32_t score;
        int32_t level;
        char name[MAX_NAME_LENGTH];
    };

    hiscore_t m_hiscores[MAX_SCORES];
    uint8_t m_joyState[JOY_AIMS];
    uint8_t m_buttonState[Button_Count];
    uint32_t m_ticks = 0;
    CAnimator *m_animator;
    CFrameSet *m_tiles = nullptr;
    CFrameSet *m_animz = nullptr;
    CFrameSet *m_annie = nullptr;
    uint8_t *m_fontData = nullptr;
    CGame *m_game = nullptr;
    CMapArch *m_maparch = nullptr;
    CRecorder *m_recorder = nullptr;
    std::vector<std::string> m_helptext;
    int m_playerFrameOffset = 0;
    int m_healthRef = 0;
    int m_countdown = 0;
    int m_scoreRank = INVALID;
    bool m_recordScore = false;
    bool m_zoom = false;
    bool m_assetPreloaded = false;
    bool m_scoresLoaded = false;
    bool m_hiscoreEnabled = false;
    bool m_paused = false;
    // Note: this has to be an int
    int m_musicMuted = false;
    Prompt m_prompt = PROMPT_NONE;
    int m_optionCooldown = 0;
    bool m_gameMenuActive = false;
    int m_gameMenuCooldown = 0;
    int m_cx;
    int m_cy;
    int m_cameraMode = CAMERA_MODE_STATIC;
    int m_currentEvent;
    int m_eventCountdown;
    int m_timer;
    int _WIDTH = 320;
    int _HEIGHT = 240;

    void drawPreScreen(CFrame &bitmap);
    void drawScreen(CFrame &bitmap);
    void fazeScreen(CFrame &bitmap, const int bitShift);
    void drawViewPortDynamic(CFrame &bitmap);
    void drawViewPortStatic(CFrame &bitmap);
    void drawLevelIntro(CFrame &bitmap);
    void drawFont(CFrame &frame, int x, int y, const char *text, Color color = WHITE, Color bgcolor = BLACK, const int scaleX = 1, const int scaleY = 1);
    void drawRect(CFrame &frame, const Rect &rect, const Color color = GREEN, bool fill = true);
    void plotLine(CFrame &frame, int x0, int y0, const int x1, const int y1, const Color color);
    inline void drawTimeout(CFrame &bitmap);
    inline void drawKeys(CFrame &bitmap);
    inline void drawSugarMeter(CFrame &bitmap, const int bx);
    inline void drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const bool alpha, const bool inverted = false, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    inline void drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const Rect &rect, const bool inverted = false, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    inline CFrame *tile2Frame(const uint8_t tileID, bool &inverted, std::unordered_map<uint32_t, uint32_t> *&colorMap);
    CFrame *specialFrame(const int aim, const uint8_t tileID);
    void nextLevel();
    void restartLevel();
    void restartGame();
    void startCountdown(int f = 1);
    int rankScore();
    void drawScores(CFrame &bitmap);
    bool inputPlayerName();
    bool handleInputString(char *inputDest, const size_t limit);
    void drawEventText(CFrame &bitmap);
    std::string getEventText(int &scaleX, int &scaleY, int &baseY, Color &color);
    void manageCurrentEvent();
    void manageTimer();
    void clearScores();
    void clearKeyStates();
    void clearJoyStates();
    void clearButtonStates();
    void manageGamePlay();
    void handleFunctionKeys();
    bool handlePrompts();
    void centerCamera();
    void moveCamera();
    int cameraSpeed() const;
    void setCameraMode(const int mode);
    void gatherSprites(std::vector<sprite_t> &sprites, const cameraContext_t &context);

    inline uint32_t fazFilter(int shift) const;
    inline int getWidth() const
    {
        return _WIDTH;
    }
    inline int getHeight() const
    {
        return _HEIGHT;
    }

    virtual void preloadAssets() = 0;
    virtual void sanityTest() = 0;
    virtual void drawHelpScreen(CFrame &bitmap);
    virtual bool loadScores() = 0;
    virtual bool saveScores() = 0;
    virtual bool read(FILE *sfile, std::string &name);
    virtual bool write(FILE *tfile, const std::string &name);
    virtual void stopMusic() = 0;
    virtual void startMusic() = 0;
    virtual void setZoom(bool zoom);
    virtual void openMusicForLevel(int i) = 0;
    virtual void setupTitleScreen() = 0;
    virtual void takeScreenshot() = 0;
    virtual void toggleFullscreen() = 0;
    virtual void manageTitleScreen() = 0;
    virtual void toggleGameMenu() = 0;
    virtual void manageGameMenu() = 0;

    virtual void manageOptionScreen() = 0;

private:
    void stopRecorder();
    void recordGame();
    void playbackGame();
};
