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
#include <vector>
#include <cstdint>
#include <cstdio>
#include "actor.h"
#include "map.h"
#include "gamesfx.h"

class CGameStats;
class CMapArch;
class ISound;

class CGame
{
public:
    enum GameMode : uint8_t
    {
        MODE_LEVEL_INTRO,
        MODE_PLAY,
        MODE_RESTART,
        MODE_GAMEOVER,
        MODE_CLICKSTART,
        MODE_HISCORES,
        MODE_IDLE,
        MODE_HELP,
        MODE_TITLE,
        MODE_TIMEOUT,
        MODE_OPTIONS,
    };

    enum : uint32_t
    {
        MAX_SUGAR_RUSH_LEVEL = 5,
        MAX_KEYS = 6,
    };

    bool loadLevel(const GameMode mode);
    bool move(const JoyAim dir);
    void manageMonsters(const int ticks);
    uint8_t managePlayer(const uint8_t *joystate);
    static Pos translate(const Pos &p, const int aim);
    void consume();
    static bool hasKey(const uint8_t c);
    void addKey(const uint8_t c);
    int goalCount() const;
    static CMap &getMap();
    void nextLevel();
    void restartLevel();
    void restartGame();
    void setMode(const GameMode mode);
    GameMode mode() const;
    bool isPlayerDead();
    void killPlayer();
    bool isGameOver() const;
    CActor &player();
    const CActor &playerConst() const;
    int score() const;
    int lives() const;
    int health() const;
    int sugar() const;
    bool hasExtraSpeed() const;
    void setMapArch(CMapArch *arch);
    void setLevel(const int levelId);
    int level() const;
    bool isGodMode() const;
    bool isRageMode() const;
    int playerSpeed() const;
    static uint8_t *keys();
    std::vector<CActor> &getMonsters();
    CActor &getMonster(int i);
    std::vector<sfx_t> &getSfx();
    void playSound(const int id) const;
    void playTileSound(const int tileID) const;
    void setLives(const int lives);
    void attach(ISound *s);
    void setSkill(const uint8_t v);
    uint8_t skill() const;
    int size() const;
    void resetStats();
    void decTimers();
    void parseHints(const char *data);
    int getEvent();
    void purgeSfx();
    static CGame *getGame();
    bool isClosure() const;
    int closusureTimer() const;
    void checkClosure();
    void decClosure();

private:
    enum
    {
        MAX_HEALTH = 200,
        DEFAULT_HEALTH = 64,
        DEFAULT_LIVES = 5,
        LEVEL_BONUS = 500,
        SCORE_LIFE = 5000,
        MAX_LIVES = 99,
        GODMODE_TIMER = 100,
        EXTRASPEED_TIMER = 200,
        CLOSURE_TIMER = 7,
        RAGE_TIMER = 150,
        DEFAULT_PLAYER_SPEED = 3,
        FAST_PLAYER_SPEED = 2,
        INVALID = -1,
        VERSION = (0x0200 << 16) + 0x0005,
        SECRET_ATTR_MIN = 0x01,
        SECRET_ATTR_MAX = 0x7f,
    };

    struct MapReport
    {
        int fruits;
        int bonuses;
        int secrets;
    };

    int m_lives = 0;
    int m_health = 0;
    int m_level = 0;
    int m_score = 0;
    int m_nextLife = SCORE_LIFE;
    int m_diamonds = 0;
    static uint8_t m_keys[MAX_KEYS];
    GameMode m_mode;
    int m_introHint = 0;
    std::vector<int> m_events;
    std::vector<CActor> m_monsters;
    std::vector<sfx_t> m_sfx;
    CActor m_player;
    CMapArch *m_mapArch = nullptr;
    ISound *m_sound = nullptr;
    std::vector<std::string> m_hints;
    CGameStats *m_gameStats;

    CGame();
    ~CGame();
    int clearAttr(const uint8_t attr);
    bool findMonsters();
    int addMonster(const CActor actor);
    int findMonsterAt(const int x, const int y) const;
    void addHealth(const int hp);
    void addPoints(const int points);
    void addLife();
    bool read(FILE *sfile);
    bool write(FILE *tfile);
    int calcScoreLife() const;
    const char *getHintText();
    bool isFruit(const uint8_t tileID) const;
    bool isBonusItem(const uint8_t tileID) const;
    void generateMapReport(MapReport &report);
    CGameStats &stats();

    friend class CGameMixin;
};
