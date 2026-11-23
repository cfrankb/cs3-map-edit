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
#include "game_auto.h"
#include "gamesfx.h"

class CGameStats;
class CMapArch;
class ISound;
class CBoss;
class Random;
class IFile;
class CAnimator;

struct TileDef;
enum Event;

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

struct scan_t
{
    bool isWater;
    bool isSolid;
    bool isDeadly;
    bool isSticky;
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
        MODE_SPLASH,
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
    // maintainer ruling 2026-11-18: hostile map files must not smuggle cell
    // values that index out of bounds into getTileDef()/g_layerdata();
    // CMap/CLayer are untouched, the engine gates its own map after every ingest
    size_t sanitiseMapTiles();
    bool move(const JoyAim dir);
    void manageMonsters(const uint32_t ticks);
    void manageBosses(const uint32_t ticks);
    void manageSpawns(const uint32_t ticks);
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
    void addHealth(const int hp, const bool silent = false); // public so tests can observe damage (see t_game_ai)
    void addPoison(const int amount);                        // adds to the S_POISON drain pool (see docs/plans/poison-damage-plan.md)
    bool isPoisoned() const;
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
    CActor &getMonster(int i);       // contract (2026-11-18 ruling): caller owns bounds checking against getMonsters().size()
    std::vector<CBoss> &getBosses(); // public so tests can inject bosses (see t_game_ai)
    void rebuildMonsterGrid();       // public so tests can sync the grid after getMonsters() injection
    std::vector<sfx_t> &getSfx();
    void playSound(const int id) const;
    void playTileSound(const int tileID) const;
    void setLives(const int lives);
    void attach(const std::shared_ptr<ISound> &s);
    void setSkill(const uint8_t v);
    uint8_t skill() const;
    int size() const;
    void resetStats();
    void decTimers();
    void parseHints(const char *data);
    int getEvent();
    void clearEvents();
    void purgeSfx();
    std::vector<fx_t> &getFx();
    int spawnFx(const int effectID, const int tileX, const int tileY);
    int spawnFxPx(const int effectID, const int px, const int py);
    // delayed fx strike (fx-boss-skills): warn the tile first (renderer
    // paints the red telegraph placeholder from pendingFx() for `ticks` =
    // the skill row's telegraphTtl), the fx spawns when the countdown
    // expires in tickFx(). Transient: NOT serialized -- save/load drops
    // pending strikes, the boss just recasts.
    struct pendingFx_t
    {
        int16_t effectId;
        int16_t tileX;
        int16_t tileY;
        int16_t ticks;
    };
    void scheduleFx(const int effectID, const int tileX, const int tileY, const int ticks);
    const std::vector<pendingFx_t> &pendingFx() const { return m_pendingFx; }
    void tickFx();
    void getFxToDraw(const fxView_t &view, const bool front,
                     std::vector<fxDraw_t> &out) const;
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
    static bool countWithinBudget(uint32_t count, long remainingBytes, uint32_t minRecordBytes);
    bool isMonsterType(const uint8_t typeID) const;
    static bool isFruit(const uint8_t tileID);
    static bool isBonusItem(const uint8_t tileID);
    static bool isBulletType(const uint8_t typeID);
    static bool isMoveableType(const uint8_t typeID);
    static bool isOneTimeItem(const uint8_t tileID);
    static bool isPushable(const uint8_t typeID);

    bool shadowActorMove(CActor &actor, const JoyAim aim);
    scan_t scanPos(const Pos &pos, const uint8_t aim) const;
    bool isGranularSolidFromPos(const Pos &pos, const uint8_t aim) const;
    CAnimator *getAnimator() { return m_animator.get(); }
    CGameStats &stats(); // public so tests can observe stats (see t_game)
    std::vector<automator_t> &automators() { return m_automators; }

    enum
    {
        INVALID = -1,
    };

private:
    Pos m_chuteTarget = {};
    // serialized after gameStats since the 0x0011 save-format bump
    // (S_CHUTE flag itself lives in gameStats)
    int m_lives = 0;
    int m_health = 0;
    int m_level = 0;
    int m_score = 0;
    int m_nextLife; // set in ctor (SCORE_LIFE is not visible in this header)
    int m_diamonds = 0;
    inline static userKeys_t m_keys;
    GameMode m_mode = MODE_TITLE;
    int m_introHint = 0;
    bool m_exitAnnounced = false; // EVENT_EXIT_OPENED fires once per level
    std::vector<Event> m_events;
    std::vector<CActor> m_monsters;
    std::vector<CBoss> m_bosses;
    std::vector<sfx_t> m_sfx;
    std::vector<fx_t> m_fx;
    std::vector<pendingFx_t> m_pendingFx; // telegraph countdowns; not saved
    uint32_t m_fxTick = 0;                // free-running fx clock (poison aura); not saved
    CActor m_player;
    CMapArch *m_mapArch = nullptr;
    std::shared_ptr<ISound> m_sound;
    std::vector<std::string> m_hints;
    std::unique_ptr<CGameStats> m_gameStats;
    std::vector<Pos> m_usedItems;
    std::unordered_map<uint16_t, int> m_monsterGrid;
    MapReport m_report = {};
    std::unique_ptr<CAnimator> m_animator;
    std::vector<automator_t> m_automators;
    std::vector<spawn_t> m_spawns;

    int m_defaultLives;
    bool m_quiet = false;
    void resetKeys();
    void clearKeyIndicators();
    void setQuiet(bool state);
    void updateMonsterGrid(const CActor &actor, const int index);
    void manageTrigger(const int x, const int y, const uint8_t attr);

    CGame();
    int clearAttr(const uint8_t attr);
    bool spawnMonsters();
    void addPoints(const int points);
    void addLife();
    int calcScoreLife() const;
    const char *getHintText();
    const CGameStats &statsConst() const;

    // regular monsters (mob)
    void handleMonster(CActor &actor, const TileDef &def);
    void handleMonsterV3(CActor &actor, const TileDef &def, const uint32_t ticks);
    void handleDrone(CActor &actor, const TileDef &def);
    void handleVamPlant(CActor &actor, const TileDef &def, std::vector<CActor> &newMonsters,
                        const std::set<int, std::greater<int>> &deletedMonsters);
    void handleCrusher(CActor &actor, const bool speeds[]);
    void handleIceCube(CActor &actor);
    void handleBullet(CActor &actor, const TileDef &def, const int i, const bulletData_t &bullet, std::set<int, std::greater<int>> &deletedMonsters);
    void handleBarrel(CActor &actor, const TileDef &def, const int i, std::set<int, std::greater<int>> &deletedMonsters);
    void handleEgg(CActor &actor, const TileDef &def);
    bool pushChain(const int x, const int y, const JoyAim aim);
    bool fuseBarrel(const Pos &pos);
    void blastRadius(const Pos &pos, const size_t radius, const int damage, std::set<int, std::greater<int>> &deletedMonsters);
    bool queueRespawn(const CActor &actor);
    bool queueRespawn(const int actorIndex);

    // boss
    CActor *spawnBullet(int x, int y, JoyAim aim, uint8_t tile);
    void handleBossPath(CBoss &boss);
    bool handleBossBullet(CBoss &boss);
    void handleBossHitboxContact(CBoss &boss, uint32_t ticks);

    inline static CMap m_map;
    friend class CGameMixin;
    friend class CRuntime;
    friend class Engine;
    friend class EngineHW;
};
