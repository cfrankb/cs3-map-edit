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
#include "colormap.h"
#include "game.h"
#include "gameui.h"
#include "shared/FileWrap.h"

#define WIDTH getWidth()
#define HEIGHT getHeight()

class CActor;
class CFrameSet;
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
    enum : int32_t
    {
        DEFAULT_WIDTH = 320,
        DEFAULT_HEIGHT = 240,
    };

#ifdef USE_QFILE
protected slots:
#endif
    virtual void mainLoop();
    void changeZoom();
    virtual void save() = 0;
    virtual void load() = 0;
    void setQuiet(bool state);

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
        TRAP_MSG_COUNTDOWN_DELAY = 2 * TICK_RATE,
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
        PLAYER_DOWN_INDEX = 8,
        PLAYER_TOTAL_FRAMES = 44,
        PLAYER_IDLE_BASE = 0x28,
        ANIMZ_INSECT1_FRAMES = 8,
        INSECT1_MAX_OFFSET = 7,
        CAMERA_MODE_STATIC = 0,
        CAMERA_MODE_DYNAMIC = 1,
        FAZ_INV_BITSHIFT = 1,
        INDEX_PLAYER_DEAD = 4,
        HEALTHBAR_CLASSIC = 0,
        HEALTHBAR_HEARTHS = 1,
    };

    enum
    {
        Annie,
        Lisa,
        Alana,
        Paul
    };

    enum : int32_t
    {
        MAX_IDLE_CYCLES = 0x100,
        IDLE_ACTIVATION = 0x40,
        MIN_WIDTH_FULL = 320,
        SUGAR_CUBES = 5,
        SCREEN_SHAKES = 4,
    };

    enum ColorMask : uint8_t
    {
        COLOR_NOCHANGE,
        COLOR_FADE,
        COLOR_INVERTED,
        COLOR_GRAYSCALE,
        COLOR_ALL_WHITE,
    };

    enum Color : uint32_t
    {
        CLEAR = 0,
        ALPHA = 0xff000000,
        WHITE = RGBA(0xff, 0xff, 0xff),          // #ffffff
        YELLOW = RGBA(0xff, 0xff, 0x00),         // #ffff00
        PURPLE = RGBA(0xff, 0x00, 0xff),         // #ff00ff
        DARKPURPLE = RGBA(0x4a, 0x45, 0x98),     // #4a4598
        BLACK = RGBA(0x00, 0x00, 0x00),          // #000000
        GREEN = RGBA(0x00, 0xff, 0x00),          // #00ff00
        DARKGREEN = RGBA(0x00, 0x80, 0x00),      // #008000
        LIME = RGBA(0xbf, 0xff, 0x00),           // #bfff00
        BLUE = RGBA(0x00, 0x00, 0xff),           // #0000ff
        MIDBLUE = RGBA(0x00, 0x00, 0x80),        // #000080
        CYAN = RGBA(0x00, 0xff, 0xff),           // #00ffff
        RED = RGBA(0xff, 0x00, 0x00),            // #ff0000
        DARKRED = RGBA(0x80, 0x00, 0x00),        // #800000
        DARKBLUE = RGBA(0x00, 0x00, 0x44),       // #000044
        DARKGRAY = RGBA(0x44, 0x44, 0x44),       // #444444
        GRAY = RGBA(0x88, 0x88, 0x88),           // #808080
        LIGHTGRAY = RGBA(0xa9, 0xa9, 0xa9),      // #a9a9a9
        ORANGE = RGBA(0xf5, 0x9b, 0x14),         // #f59b14
        DARKORANGE = RGBA(0xff, 0x8c, 0x00),     // #ff8c00
        CORAL = RGBA(0xff, 0x7f, 0x50),          // #ff7f50
        PINK = RGBA(0xff, 0xc0, 0xcb),           // #ffc0cb
        HOTPINK = RGBA(0xff, 0x69, 0xb4),        // #ff69b4
        DEEPPINK = RGBA(0xff, 0x14, 0x93),       // #ff1493
        OLIVE = RGBA(0x80, 0x80, 0x00),          // #808000
        MEDIUMSEAGREEN = RGBA(0x3C, 0xB3, 0x71), // #3CB371
        SEAGREEN = RGBA(0x2E, 0x8B, 0x57),       // #2E8B57
        BLUEVIOLET = RGBA(0x8A, 0x2B, 0xE2),     // #8A2BE2
        DEEPSKYBLUE = RGBA(0x00, 0xBF, 0xFF),    // #00BFFF
        LAVENDER = RGBA(0xE6, 0xE6, 0xFA),       // #E6E6FA
        DARKSLATEGREY = RGBA(0x2F, 0x4F, 0x4F),  // #2F4F4F
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
        BUTTON_BACK,
        BUTTON_GUIDE,
        BUTTON_START,
        LEFTSTICK,
        RIGHTSTICK,
        LEFTSHOULDER,
        RIGHTSHOULDER,
        Button_Count,
        DPAD_UP = 12,
        DPAD_DOWN = 13,
        DPAD_LEFT = 14,
        DPAD_RIGHT = 15,
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
        uint8_t attr;
    };

    struct visualCues_t
    {
        bool diamondShimmer;
        bool livesShimmer;
    };

    struct visualStates_t
    {
        int rGoalCount = 0;
        int rLives = 0;
        int rSugar = 0;
        int sugarFx = 0;
        uint8_t sugarCubes[SUGAR_CUBES];
    };

    struct hiscore_t
    {
        int32_t score;
        int32_t level;
        char name[MAX_NAME_LENGTH];
    };

    hiscore_t m_hiscores[MAX_SCORES];
    uint8_t m_joyState[JOY_AIMS];
    uint8_t m_vjoyState[JOY_AIMS];
    uint8_t m_buttonState[Button_Count];
    uint32_t m_ticks = 0;
    std::unique_ptr<CAnimator> m_animator;
    std::unique_ptr<CFrameSet> m_tiles;
    std::unique_ptr<CFrameSet> m_animz;
    std::unique_ptr<CFrameSet> m_users;
    std::vector<uint8_t> m_fontData;
    CGame *m_game = nullptr;
    CMapArch *m_maparch = nullptr;
    std::unique_ptr<CRecorder> m_recorder;
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
    bool m_summaryEnabled = false;
    bool m_paused = false;
    int m_musicMuted = false; // Note: this has to be an int
    Prompt m_prompt = PROMPT_NONE;
    int m_optionCooldown = 0;
    bool m_gameMenuActive = false;
    int m_gameMenuCooldown = 0;
    int m_cx;
    int m_cy;
    int m_cameraMode = CAMERA_MODE_STATIC;
    int m_healthBar = HEALTHBAR_CLASSIC;
    int m_currentEvent;
    int m_eventCountdown;
    int m_timer;
    int _WIDTH = DEFAULT_WIDTH;
    int _HEIGHT = DEFAULT_HEIGHT;
    ColorMaps m_colormaps;
    visualStates_t m_visualStates;
    CGameUI m_ui;
    CFileWrap m_recorderFile;
    bool m_quiet = false;

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
    inline void drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const bool alpha, const ColorMask colorMask = COLOR_NOCHANGE, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    inline void drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const Rect &rect, const ColorMask colorMask = COLOR_NOCHANGE, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    void drawTileFaz(CFrame &bitmap, const int x, const int y, CFrame &tile, int fazBitShift = 0, const ColorMask colorMask = COLOR_NOCHANGE);
    inline CFrame *tile2Frame(const uint8_t tileID, ColorMask &colorMask, std::unordered_map<uint32_t, uint32_t> *&colorMap);
    void drawHealthBar(CFrame &bitmap, const bool isPlayerHurt);
    void drawGameStatus(CFrame &bitmap, const visualCues_t &visualcues);
    CFrame *calcSpecialFrame(const sprite_t &sprite);
    void nextLevel();
    void restartLevel();
    void restartGame();
    void startCountdown(int f = 1);
    int rankUserScore();
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
    void handleFunctionKeys_Game(int k);
    void handleFunctionKeys_General(int k);
    bool handlePrompts();
    void centerCamera();
    void moveCamera();
    int cameraSpeed() const;
    void setCameraMode(const int mode);
    void gatherSprites(std::vector<sprite_t> &sprites, const cameraContext_t &context);
    void beginLevelIntro(CGame::GameMode mode);
    void clearVisualStates();
    void initUI();
    void drawUI(CFrame &bitmap, CGameUI &ui);
    int whichButton(CGameUI &ui, int x, int y);

    constexpr inline uint32_t fazFilter(const int bitShift) const
    {
        return (0xff >> bitShift) << 16 |
               (0xff >> bitShift) << 8 |
               0xff >> bitShift;
    }

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
    virtual bool read(IFile &sfile, std::string &name);
    virtual bool write(IFile &tfile, const std::string &name);
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
    virtual void manageUserMenu() = 0;
    virtual void manageLevelSummary() = 0;
    virtual void initLevelSummary() = 0;
    virtual void changeMoodMusic(CGame::GameMode mode) = 0;
    virtual void manageSkillMenu() = 0;

private:
    void stopRecorder();
    void recordGame();
    void playbackGame();
};
