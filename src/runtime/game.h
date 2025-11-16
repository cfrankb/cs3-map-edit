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
#include <string>
#include <set>
#include <unordered_map>
#include <memory>
#include "actor.h"
#include "map.h"
#include "events.h"

class CGameStats;
class CMapArch;
class ISound;
class CBoss;
class Random;
class IFile;
struct TileDef;
enum Event;
enum Sfx : uint16_t;
struct sfx_t;

struct MapReport
{
    int fruits;
    int bonuses;
    int secrets;
};

struct bulletData_t
{
    uint8_t sound;
    Sfx sfxID;
    uint16_t sfxTimeOut;
};

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
        MODE_USERSELECT,
        MODE_LEVEL_SUMMARY,
        MODE_SKLLSELECT,
        MODE_NEW_INPUTNAME,
        MODE_CHUTE,
        MODE_TEST,
    };

    enum : uint32_t
    {
        MAX_SUGAR_RUSH_LEVEL = 5,
        MAX_KEYS = 6,
        MAX_KEY_STATE = 5,
    };

    enum Hurt
    {
        HurtNone,
        HurtFaz,
        HurtInv,
        HurtFlash,
        HurtStart = HurtFlash
    };

    struct userKeys_t
    {
        uint8_t tiles[MAX_KEYS];
        uint8_t indicators[MAX_KEYS];
    };

    ~CGame();
    bool loadLevel(const GameMode mode);
    bool move(const JoyAim dir);
    void manageMonsters(const int ticks);
    void manageBosses(const int ticks);
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
    static userKeys_t &keys();
    std::vector<CActor> &getMonsters();
    CActor &getMonster(int i);
    std::vector<sfx_t> &getSfx();
    void playSound(const int id) const;
    void playTileSound(const int tileID) const;
    void setLives(const int lives);
    void attach(std::shared_ptr<ISound> &s);
    void setSkill(const uint8_t v);
    uint8_t skill() const;
    int size() const;
    void resetStats();
    void decTimers();
    void parseHints(const char *data);
    int getEvent();
    void purgeSfx();
    static CGame *getGame();
    static void destroy();
    bool isClosure() const;
    bool isLevelCompleted() const;
    int closusureTimer() const;
    void checkClosure();
    void decClosure();
    bool isFrozen() const;
    int maxHealth() const;
    int getUserID() const;
    void setUserID(const int userID) const;
    static MapReport generateMapReport(CMap &map);
    MapReport currentMapReport();
    const MapReport &originalMapReport();
    int timeTaken();
    void incTimeTaken();
    void resetStatsUponDeath();
    void decKeyIndicators();
    int defaultLives();
    void setDefaultLives(int lives);
    bool read(IFile &sfile);
    bool write(IFile &tfile);
    const std::vector<CBoss> &bosses();
    int findMonsterAt(const int x, const int y) const;
    void deleteMonster(const int i);
    static Random &getRandom();
    static bool validateSignature(const char *signature, const uint32_t version);
    bool isMonsterType(const uint8_t typeID) const;
    static bool isFruit(const uint8_t tileID);
    static bool isBonusItem(const uint8_t tileID);
    static bool isBulletType(const uint8_t typeID);
    static bool isMoveableType(const uint8_t typeID);
    static bool isOneTimeItem(const uint8_t tileID);
    static bool isPushable(const uint8_t typeID);

    bool shadowActorMove(CActor &actor, const JoyAim aim);

    enum
    {
        INVALID = -1,
    };

private:
    Pos m_chuteTarget;
    int m_lives = 0;
    int m_health = 0;
    int m_level = 0;
    int m_score = 0;
    int m_nextLife;
    int m_diamonds = 0;
    inline static userKeys_t m_keys;
    GameMode m_mode;
    int m_introHint = 0;
    std::vector<Event> m_events;
    std::vector<CActor> m_monsters;
    std::vector<CBoss> m_bosses;
    std::vector<sfx_t> m_sfx;
    CActor m_player;
    CMapArch *m_mapArch = nullptr;
    std::shared_ptr<ISound> m_sound;
    std::vector<std::string> m_hints;
    std::unique_ptr<CGameStats> m_gameStats;
    std::vector<Pos> m_usedItems;
    std::unordered_map<uint16_t, int> m_monsterGrid;
    MapReport m_report;
    int m_defaultLives;
    bool m_quiet = false;
    void resetKeys();
    void clearKeyIndicators();
    void setQuiet(bool state);
    void rebuildMonsterGrid();
    void updateMonsterGrid(const CActor &actor, const int index);

    CGame();
    int clearAttr(const uint8_t attr);
    bool spawnMonsters();
    void addHealth(const int hp);
    void addPoints(const int points);
    void addLife();
    int calcScoreLife() const;
    const char *getHintText();
    CGameStats &stats();
    const CGameStats &statsConst() const;

    // regular monsters (mob)
    void handleMonster(CActor &actor, const TileDef &def);
    void handleDrone(CActor &actor, const TileDef &def);
    void handleVamPlant(CActor &actor, const TileDef &def, std::vector<CActor> &newMonsters);
    void handleCrusher(CActor &actor, const bool speeds[]);
    void handleIceCube(CActor &actor);
    void handleBullet(CActor &actor, const TileDef &def, const int i, const bulletData_t &bullet, std::set<int, std::greater<int>> &deletedMonsters);
    void handleBarrel(CActor &actor, const TileDef &def, const int i, std::set<int, std::greater<int>> &deletedMonsters);
    bool pushChain(const int x, const int y, const JoyAim aim);
    bool fuseBarrel(const Pos &pos);
    void blastRadius(const Pos &pos, const size_t radius, const int damage, std::set<int, std::greater<int>> &deletedMonsters);

    // boss
    CActor *spawnBullet(int x, int y, JoyAim aim, uint8_t tile);
    void handleBossPath(CBoss &boss);
    bool handleBossBullet(CBoss &boss);
    void handleBossHitboxContact(CBoss &boss);

    inline static CMap m_map;
    friend class CGameMixin;
};
