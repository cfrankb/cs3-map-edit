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
#include <mutex>
#include "colormap.h"
#include "game.h"
#include "gameui.h"
#include "rect.h"
#include "color.h"
#include "colormasks.h"
#include "shared/FileWrap.h"

class CActor;
class CFrameSet;
class CFrame;
class CMapArch;
class CAnimator;
class IMusic;
class CRecorder;

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
    void setQuiet(bool state);

    enum
    {
        MAX_NAME_LENGTH = 16
    };

    struct hiscore_t
    {
        int32_t score;
        int32_t level;
        char name[MAX_NAME_LENGTH];
    };

    hiscore_t &getHiScoreAtIndex(int i) { return m_hiscores[i]; }
    int scoreRank() { return m_scoreRank; }
    bool recordScore() { return m_recordScore; }
    std::vector<std::string> &helptext() { return m_helptext; }

protected:
    enum : uint32_t
    {
        TICK_RATE = 24, // 1s
        NO_ANIMZ = 255,
        KEY_PRESSED = 1,
        KEY_RELEASED = 0,
        BUTTON_PRESSED = 1,
        BUTTON_RELEASED = 0,
        INTRO_DELAY = 3 * TICK_RATE,
        HISCORE_DELAY = 5 * TICK_RATE,
        EVENT_COUNTDOWN_DELAY = TICK_RATE,
        MSG_COUNTDOWN_DELAY = 6 * TICK_RATE,
        TRAP_MSG_COUNTDOWN_DELAY = 2 * TICK_RATE,
        TILE_SIZE = 16,
        COUNTDOWN_INTRO = 1,
        COUNTDOWN_RESTART = 2,
        GAME_MENU_COOLDOWN = 10,
        FONT_SIZE = 8,
        MAX_SCORES = 18,
        KEY_REPETE_DELAY = 5,
        KEY_NO_REPETE = 1,
        SAVENAME_PTR_OFFSET = 8,
        CARET_SPEED = 8,
        INTERLINES = 2,
        Y_STATUS = 2,
        PLAYER_FRAMES = 8,
        PLAYER_STD_FRAMES = 7,
        PLAYER_DOWN_INDEX = 8,
        PLAYER_TOTAL_FRAMES = 45,
        PLAYER_BOAT_FRAME = 44,
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

    enum : int32_t
    {
        MAX_IDLE_CYCLES = 0x100,
        IDLE_ACTIVATION = 0x40,
        MIN_WIDTH_FULL = 320,
        SUGAR_CUBES = 5,
        SCREEN_SHAKES = 4,
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

    struct cameraContext_t
    {
        int mx;
        int ox;
        int my;
        int oy;
    };

    struct sprite_t
    {
        int16_t x;
        int16_t y;
        uint16_t tileID;
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

    struct message_t
    {
        int scaleX;
        int scaleY;
        int baseY;
        Color color;
        std::string lines[3];
    };

    hiscore_t m_hiscores[MAX_SCORES];
    uint8_t m_joyState[JOY_AIMS];
    uint8_t m_vjoyState[JOY_AIMS];
    uint8_t m_buttonState[Button_Count];
    uint32_t m_ticks = 0;
    CAnimator *m_animator;
    std::unique_ptr<CFrameSet> m_tiles;
    std::unique_ptr<CFrameSet> m_animz;
    std::unique_ptr<CFrameSet> m_users;
    std::unique_ptr<CFrameSet> m_sheet0;
    std::unique_ptr<CFrameSet> m_sheet1;
    std::unique_ptr<CFrameSet> m_uisheet;
    std::unique_ptr<CFrameSet> m_layerTiles;
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
    // bool m_gameMenuActive = false;
    int m_gameMenuCooldown = 0;
    int m_cx;
    int m_cy;
    int m_cameraMode = CAMERA_MODE_STATIC;
    int m_healthBar = HEALTHBAR_CLASSIC;
    int m_currentEvent;
    int m_eventCountdown;
    int m_timer;
    int _WIDTH;
    int _HEIGHT;
    ColorMaps m_colormaps;
    visualStates_t m_visualStates;
    CGameUI m_ui;
    CFileWrap m_recorderFile;
    bool m_quiet = false;
    std::mutex m_mutex;

    void drawPreScreen(CFrame &bitmap);
    void drawScreen(CFrame &bitmap);
    void fazeScreen(CFrame &bitmap, const int bitShift);
    void flashScreen(CFrame &bitmap);
    void drawViewPortDynamic(CFrame &bitmap);
    void drawViewPortStatic(CFrame &bitmap);
    void drawBossses(CFrame &bitmap, const int mx, const int my, const int sx, const int sy);
    void drawLevelIntro(CFrame &bitmap);
    void drawFont(CFrame &frame, int x, int y, const char *text, Color color = WHITE, Color bgcolor = BLACK, const int scaleX = 1, const int scaleY = 1);
    void drawFont6x6(CFrame &frame, int x, int y, const char *text, const Color color = WHITE, const Color bgcolor = BLACK);
    void drawRect(CFrame &frame, const rect_t &rect, const Color color = GREEN, bool fill = true);
    void plotLine(CFrame &frame, int x0, int y0, const int x1, const int y1, const Color color);
    inline void drawTimeout(CFrame &bitmap);
    inline void drawKeys(CFrame &bitmap);
    inline void drawSugarMeter(CFrame &bitmap, const int bx);
    inline void drawTile(CFrame &bitmap, const int x, const int y, const CFrame &tile, const bool alpha, const ColorMask colorMask = COLOR_NOCHANGE, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    inline void drawTile(CFrame &bitmap, const int x, const int y, CFrame &tile, const rect_t &rect, const ColorMask colorMask = COLOR_NOCHANGE, std::unordered_map<uint32_t, uint32_t> *colorMap = nullptr);
    void drawTileFaz(CFrame &bitmap, const int x, const int y, CFrame &tile, int fazBitShift = 0, const ColorMask colorMask = COLOR_NOCHANGE);
    inline CFrame *tile2Frame(const uint8_t tileID, ColorMask &colorMask, std::unordered_map<uint32_t, uint32_t> *&colorMap);
    void drawHealthBar(CFrame &bitmap, const bool isPlayerHurt);
    void drawGameStatus(CFrame &bitmap, const visualCues_t &visualcues);
    void drawScroll(CFrame &bitmap);
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
    message_t getEventText(const int baseY);
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

    constexpr inline uint32_t
    fazFilter(const int bitShift) const
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
    virtual bool manageGameMenu() = 0;
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
