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
#include <unordered_map>
#include <cstring>
#include <stdarg.h>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <array>
#include "game.h"
#include "map.h"
#include "actor.h"
#include "sprtypes.h"
#include "tilesdata.h"
#include "maparch.h"
#include "shared/IFile.h"
#include "shared/interfaces/ISound.h"
#include "skills.h"
#include "events.h"
#include "sounds.h"
#include "states.h"
#include "statedata.h"
#include "gamestats.h"
#include "attr.h"
#include "logger.h"
#include "strhelper.h"
#include "bossdata.h"
#include "randomz.h"
#include "filemacros.h"
#include "gamesfx.h"
#include "boss.h"
#include "tilesdefs.h"
#include "layerdata.h"
#include "animator.h"

namespace GamePrivate
{
    constexpr uint32_t ENGINE_VERSION = (0x0200 << 16) + 0x0009;
    constexpr const char GAME_SIGNATURE[]{'C', 'S', '3', 'b'};
    Random g_randomz(12345, 0);

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
        FREEZE_TIMER = 80,
        TRAP_DAMAGE = -16,
        DEFAULT_PLAYER_SPEED = 3,
        FAST_PLAYER_SPEED = 2,
        MAX_SHIELDS = 2,
        MAX_FACTOR = 4,
        BARREL_TTL = 20,
        AUTOKILL = -1024,
    };
}

using namespace GamePrivate;

/**
 * @brief Construct a new CGame::CGame object
 *
 */
CGame::CGame()
{
    LOGI("starting up engine: 0x%.8x", ENGINE_VERSION);
    m_health = 0;
    m_level = 0;
    m_score = 0;
    m_gameStats = std::make_unique<CGameStats>();
    m_gameStats->set(S_SKILL, SKILL_EASY);
    m_defaultLives = DEFAULT_LIVES;
    m_nextLife = SCORE_LIFE;
    m_lives = defaultLives();
    m_animator = std::make_unique<CAnimator>();
}

/**
 * @brief Destroy the CGame::CGame object
 *
 */
CGame::~CGame()
{
    m_sound.reset();
}

/**
 * @brief returns the current map
 *
 * @return CMap&
 */
CMap &CGame::getMap()
{
    return m_map;
}

/**
 * @brief return a player instance
 *
 * @return const CActor&
 */
CActor &CGame::player()
{
    return m_player;
}

/**
 * @brief return a player instance that cannot be modified
 *
 * @return const CActor&
 */
const CActor &CGame::playerConst() const
{
    return m_player;
}

bool CGame::fuseBarrel(const Pos &pos)
{
    const int i = findMonsterAt(pos.x, pos.y);
    if (i != INVALID)
    {
        // arm the fuse
        CActor &barrel = m_monsters[i];
        if (barrel.getTTL() == CActor::NoTTL)
        {
            barrel.setTTL(BARREL_TTL);
            if (pos.y > 0)
                m_sfx.emplace_back(sfx_t{
                    .x = pos.x,
                    .y = static_cast<int16_t>(pos.y - 1),
                    .sfxID = SFX_FLAME,
                    .timeout = SFX_FLAME_TIMEOUT,
                });
        }
        return true;
    }
    return false;
}

/**
 * @brief move player in a give direction
 *
 * @param aim
 * @return true
 * @return false
 */
bool CGame::move(const JoyAim aim)
{
    const uint8_t tileID = m_player.tileAt(aim);
    const TileDef &def = getTileDef(tileID);
    if (m_player.canMove(aim))
    {
        m_player.move(aim);
        consume();
        return true;
    }
    else if (isPushable(def.type))
    {
        Pos pushPos = translate(m_player.pos(), aim);
        if (pushChain(pushPos.x, pushPos.y, aim))
        {
            m_player.move(aim);
            return true;
        }
    }
    else if (def.type == TYPE_BARREL)
    {
        const Pos pos = CGame::translate(m_player.pos(), aim);
        fuseBarrel(pos);
    }
    return false;
}

/**
 * @brief player consumes tile at current position
 *
 */
void CGame::consume()
{
    const uint8_t pu = m_player.getPU();
    const TileDef &def = getTileDef(pu);
    const auto &result = scanPos(m_player.pos());
    const bool isWater = result.isWater || def.type == TYPE_SWAMP;
    if (def.type == TYPE_PICKUP)
    {
        addPoints(def.score);
        m_player.setPU(TILES_BLANK);
        addHealth(def.health);
        playTileSound(pu);
    }
    else if (def.type == TYPE_KEY)
    {
        addPoints(def.score);
        m_player.setPU(TILES_BLANK);
        addKey(pu);
        addHealth(def.health);
        playSound(SOUND_KEY);
    }
    else if (def.type == TYPE_DIAMOND)
    {
        addPoints(def.score);
        m_player.setPU(TILES_BLANK);
        --m_diamonds;
        checkClosure();
        addHealth(def.health);
        playTileSound(pu);
    }
    else if (isWater && m_gameStats->get(S_BOAT) == 0)
    {
        // apply swamp damage if no boat
        addHealth(def.health);
    }
    else if (def.type == TYPE_CHUTE)
    {
        m_gameStats->set(S_CHUTE, 1);
        checkClosure();
    }
    else if (def.type == TYPE_FIRE)
    {
        addHealth(def.health);
    }

    if (result.isDeadly)
    {
        addHealth(AUTOKILL);
    }

    if (isFruit(pu))
    {
        auto &sugar = m_gameStats->get(S_SUGAR);
        ++sugar;
        if (sugar != MAX_SUGAR_RUSH_LEVEL)
            m_events.emplace_back(EVENT_SUGAR);
    }

    if (pu == TILES_SHIELD)
    {
        if (m_gameStats->get(S_SHIELD) < MAX_SHIELDS)
            m_gameStats->inc(S_SHIELD);
        m_events.emplace_back(EVENT_SHIELD);
    }

    if (pu == TILES_BOAT)
    {
        m_gameStats->set(S_BOAT, 1);
        m_events.emplace_back(EVENT_BOAT);
    }

    if (isOneTimeItem(pu))
        m_usedItems.push_back(m_player.pos());

    // apply flags
    if (def.flags & FLAG_EXTRA_LIFE)
    {
        m_events.emplace_back(EVENT_EXTRA_LIFE);
        addLife();
    }

    if (def.flags & FLAG_GODMODE)
    {
        if (!m_gameStats->get(S_GOD_MODE_TIMER))
            playSound(SOUND_POWERUP3);
        m_gameStats->set(S_GOD_MODE_TIMER, GODMODE_TIMER);
        m_events.emplace_back(EVENT_GOD_MODE);
    }
    else if (def.flags & FLAG_EXTRA_SPEED || m_gameStats->get(S_SUGAR) == MAX_SUGAR_RUSH_LEVEL)
    {
        if (!m_gameStats->get(S_EXTRA_SPEED_TIMER))
            playSound(SOUND_POWERUP2);
        const int sugarLevel = std::min(m_gameStats->inc(S_SUGAR_LEVEL), (int)MAX_SUGAR_RUSH_LEVEL);
        m_gameStats->set(S_EXTRA_SPEED_TIMER, (int)EXTRASPEED_TIMER * 1.5 * sugarLevel);
        m_events.emplace_back(EVENT_SUGAR_RUSH);
        m_gameStats->set(S_SUGAR, 0);
    }
    else if (def.flags & FLAG_RAGE)
    {
        if (!m_gameStats->get(S_RAGE_TIMER))
            playSound(SOUND_POWERUP3);
        m_gameStats->set(S_RAGE_TIMER, RAGE_TIMER);
        m_events.emplace_back(EVENT_RAGE);
    }

    // trigger key
    int x = m_player.x();
    int y = m_player.y();
    uint8_t attr = m_map.getAttr(x, y);
    m_map.setAttr(x, y, 0);
    if (attr == ATTR_FREEZE_TRAP)
    {
        m_gameStats->set(S_FREEZE_TIMER, FREEZE_TIMER);
        m_events.emplace_back(EVENT_FREEZE);
    }
    else if (attr == ATTR_TRAP)
    {
        m_events.emplace_back(EVENT_TRAP);
        addHealth(TRAP_DAMAGE);
    }
    else if (RANGE(attr, PASSAGE_ATTR_MIN, PASSAGE_ATTR_MAX))
    {
        if (clearAttr(attr))
        {
            playSound(SOUND_0009);
            if (RANGE(attr, SECRET_ATTR_MIN, SECRET_ATTR_MAX))
                m_events.emplace_back(EVENT_SECRET);
            else
                m_events.emplace_back(EVENT_PASSAGE);
        }
    }
    else if (attr >= MSG0 && m_map.states().hasS(attr))
    {
        // Messsage Event (scrolls, books etc)
        m_events.emplace_back(static_cast<Event>(attr));
    }
}

/**
 * @brief Load Level from disk and set the game mode
 *
 * @param mode - MODE_INTRO_LEVEL, MODE_RESTART, MODE_TIMEOUT
 * @return true - upon success
 * @return false
 */
bool CGame::loadLevel(const GameMode mode)
{
    if (!m_quiet)
        LOGI("loading level: %d ...", m_level + 1);
    setMode(mode);

    // clear used items list when entering a new level
    if (mode == MODE_CHUTE || mode == MODE_LEVEL_INTRO)
        m_usedItems.clear();

    // extract level from MapArch
    m_map = *(m_mapArch->at(m_level));

    // remove used item
    for (const auto &pos : m_usedItems)
        m_map.set(pos.x, pos.y, TILES_BLANK);

    if (!m_quiet)
        LOGI("level loaded");

    if (m_hints.size() == 0)
        LOGW("hints not loaded -- not available???");

    m_introHint = m_hints.size() ? rand() % m_hints.size() : 0;

    // clear event list
    clearEvents();

    // Use origin pos if available
    CStates &states = m_map.states();
    const uint16_t origin = states.getU(POS_ORIGIN);
    Pos pos;
    if (m_gameStats->get(S_CHUTE) != 0)
    {
        pos = m_chuteTarget;
        if (!m_map.isValid(pos.x, pos.y))
        {
            pos = m_map.findFirst(TILES_ANNIE2);
        }
        else
        {
            m_map.replaceTile(TILES_ANNIE2, TILES_BLANK);
            m_map.set(pos.x, pos.y, TILES_ANNIE2);
        }
    }
    else if (origin != 0)
    {
        pos = CMap::toPos(origin);
        m_map.set(pos.x, pos.y, TILES_ANNIE2);
    }
    else
    {
        pos = m_map.findFirst(TILES_ANNIE2);
    }
    if (!m_quiet)
        LOGI("Player at: %d %d", pos.x, pos.y);
    m_player = CActor(pos, TYPE_PLAYER, AIM_DOWN);
    m_diamonds = states.hasU(MAP_GOAL) ? states.getU(MAP_GOAL) : m_map.count(TILES_DIAMOND);
    resetKeys();
    m_health = DEFAULT_HEALTH;
    spawnMonsters();
    m_sfx.clear();
    resetStats();
    m_report = currentMapReport();
    return true;
}

/**
 * @brief Increase Level by 1. Wraps back to 0.
 *
 */
void CGame::nextLevel()
{
    addPoints(LEVEL_BONUS + m_health);
    addPoints(m_map.states().getU(TIMEOUT) * 2);
    if (m_level != static_cast<int>(m_mapArch->size()) - 1)
    {
        ++m_level;
    }
    else
    {
        m_level = 0;
    }
}

/**
 * @brief restarts level and clears player that are reset upon death
 *
 */
void CGame::restartLevel()
{
    clearEvents();
    resetStats();
    resetStatsUponDeath();
}

/**
 * @brief restart game and reset player state
 *
 */
void CGame::restartGame()
{
    resetStats();
    m_level = 0;
    m_nextLife = calcScoreLife();
    m_score = 0;
    m_lives = defaultLives();
    resetStatsUponDeath();
}

/**
 * @brief Reset SugarMeter to 0.
 *
 */
void CGame::resetStatsUponDeath()
{
    m_gameStats->set(S_SUGAR, 0);
    m_gameStats->set(S_SUGAR_LEVEL, 0);
    m_gameStats->set(S_SHIELD, 0);
    m_gameStats->set(S_BOAT, 0);
}

/**
 * @brief Decrenent Powerup timers
 *
 */
void CGame::decTimers()
{
    constexpr const GameStat stats[] = {
        S_GOD_MODE_TIMER,
        S_EXTRA_SPEED_TIMER,
        S_RAGE_TIMER,
        S_FREEZE_TIMER,
        S_FLASH,
    };
    for (const auto &stat : stats)
    {
        m_gameStats->dec(stat);
    }
}

/**
 * @brief reset player stats - called to clear stats when the level is loaded
 *
 */
void CGame::resetStats()
{
    constexpr const GameStat stats[] = {
        S_GOD_MODE_TIMER,
        S_EXTRA_SPEED_TIMER,
        S_RAGE_TIMER,
        S_CLOSURE,
        S_CLOSURE_TIMER,
        S_REVEAL_EXIT,
        S_IDLE_TIME,
        S_FREEZE_TIMER,
        S_TIME_TAKEN,
        S_CHUTE,
        S_FLASH,
    };
    for (const auto &stat : stats)
    {
        m_gameStats->set(stat, 0);
    }
}

/**
 * @brief set current level
 *
 * @param levelId
 */
void CGame::setLevel(int levelId)
{
    m_level = levelId;
}

/**
 * @brief  Returns current level
 *
 * @return int
 */
int CGame::level() const
{
    return m_level;
}

/**
 * @brief Set Map Arch
 *
 * @param arch - the wrapper for the mapz file
 */
void CGame::setMapArch(CMapArch *arch)
{
    m_mapArch = arch;
}

bool CGame::isMonsterType(const uint8_t typeID) const
{
    std::array<uint8_t, 8> monsterTypes = {
        TYPE_MONSTER,
        TYPE_VAMPLANT,
        TYPE_DRONE,
        TYPE_ICECUBE,
        TYPE_BOULDER,
        TYPE_FIREBALL,
        TYPE_LIGHTNING_BOLT,
        TYPE_BARREL,
    };

    for (size_t i = 0; i < monsterTypes.size(); ++i)
        if (monsterTypes[i] == typeID)
            return true;
    return false;
}

/**
 * @brief Create a list of monster contained on the level
 *
 * @return true
 * @return false
 */
bool CGame::spawnMonsters()
{
    m_monsters.clear();
    m_bosses.clear();
    for (int y = 0; y < m_map.height(); ++y)
    {
        for (int x = 0; x < m_map.width(); ++x)
        {
            uint8_t c = m_map.at(x, y);
            const TileDef &def = getTileDef(c);
            if (isMonsterType(def.type))
            {
                if (isPushable(def.type))
                    m_monsters.emplace_back(CActor(x, y, def.type, JoyAim::AIM_NONE));
                else
                    m_monsters.emplace_back(CActor(x, y, def.type));
            }
        }
    }

    std::vector<Pos> removed;
    for (const auto &[key, attr] : m_map.attrs())
    {
        if (RANGE(attr, ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX))
        {
            const Pos &pos = CMap::toPos(key);
            const JoyAim aim = attr < ATTR_CRUSHERH_MIN ? AIM_UP : AIM_LEFT;
            m_monsters.emplace_back(CActor(pos, attr, aim));
            removed.emplace_back(pos);
        }
        else if (RANGE(attr, ATTR_BOSS_MIN, ATTR_BOSS_MAX))
        {
            const Pos &pos = CMap::toPos(key);
            const bossData_t *bossData = getBossData(attr);
            if (bossData)
            {
                CBoss boss(
                    static_cast<int16_t>(pos.x) * CBoss::BOSS_GRANULAR_FACTOR,
                    static_cast<int16_t>(pos.y) * CBoss::BOSS_GRANULAR_FACTOR,
                    bossData);
                LOGI("boss 0x%.2x speed=%d", attr, bossData->speed);
                m_bosses.emplace_back(std::move(boss));
            }
            else
            {
                LOGW("ignored spawn point for unhandled boss type: 0x%.2x", attr);
            }
            removed.emplace_back(pos);
        }
    }

    for (const auto &pos : removed)
    {
        m_map.setAttr(pos.x, pos.y, 0);
    }

    // create a map of all monsters
    rebuildMonsterGrid();

    if (!m_quiet)
    {
        LOGI("%zu actors found.", m_monsters.size());
        LOGI("%zu bosses found.", m_bosses.size());
    }
    return true;
}

/**
 * @brief Find MonsterId at given pos(x,y)
 *
 * @param x position X
 * @param y position Y
 * @return int actorID, INVALID if not found
 */
int CGame::findMonsterAt(const int x, const int y) const
{
    if (!m_map.isValid(x, y))
        return INVALID;
    uint16_t key = CMap::toKey(x, y);
    auto it = m_monsterGrid.find(key);
    return it != m_monsterGrid.end() ? it->second : INVALID;
}

/**
 * @brief Manage player
 *
 * @param joystate
 * @return uint8_t
 */
uint8_t CGame::managePlayer(const uint8_t *joystate)
{
    auto const pu = m_player.getPU();
    const TileDef &def = getTileDef(pu);
    const auto &result = scanPos(m_player.pos());
    const bool isWater = result.isWater || def.type == TYPE_SWAMP;
    if (result.isDeadly)
    {
        addHealth(AUTOKILL);
    }
    else if (isWater && m_gameStats->get(S_BOAT) == 0)
    {
        // apply health damage
        addHealth(def.health);
    }
    else if (pu == TILES_FLAME)
    {
        // apply health damage
        addHealth(def.health);
    }
    const JoyAim aims[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
    for (uint8_t i = 0; i < TOTAL_AIMS; ++i)
    {
        const JoyAim aim = aims[i];
        if (joystate[aim] && move(aim))
        {
            return aim;
        }
    }
    return AIM_NONE;
}

/**
 * @brief Alter the position to reflect a move in a given direct
 *
 * @param p original positon
 * @param aim direct UP,DOWN,LEFT or RIGHT
 * @return Pos new position
 */
Pos CGame::translate(const Pos &p, const int aim)
{
    Pos t = p;

    switch (aim)
    {
    case AIM_UP:
        if (t.y > 0)
        {
            --t.y;
        }
        break;
    case AIM_DOWN:
        if (t.y < m_map.height() - 1)
        {
            ++t.y;
        }
        break;
    case AIM_LEFT:
        if (t.x > 0)
        {
            --t.x;
        }
        break;
    case AIM_RIGHT:
        if (t.x < m_map.width() - 1)
        {
            ++t.x;
        }
    }

    return t;
}

/**
 * @brief checks if the player has a given key
 *
 * @param c tileID
 * @return true
 * @return false
 */
bool CGame::hasKey(const uint8_t c)
{
    for (uint32_t i = 0; i < MAX_KEYS; ++i)
    {
        if (m_keys.tiles[i] == c)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief add key to player inventory
 *
 * @param c tileID
 */
void CGame::addKey(const uint8_t c)
{
    for (uint32_t i = 0; i < sizeof(m_keys); ++i)
    {
        if (m_keys.tiles[i] == c)
        {
            break;
        }
        if (m_keys.tiles[i] == '\0')
        {
            m_keys.tiles[i] = c;
            m_keys.indicators[i] = MAX_KEY_STATE;
            break;
        }
    }
}

/**
 * @brief Current diamond count on map
 *
 * @return int
 */
int CGame::goalCount() const
{
    return m_diamonds;
}

/**
 * @brief clear all instance of a given attribute on the map
 *        allows for secret passages
 *
 * @param attr
 * @return int
 */
int CGame::clearAttr(const uint8_t attr)
{
    std::vector<uint16_t> keys;
    int count = 0;
    for (const auto &[key, tileAttr] : m_map.attrs())
    {
        if (tileAttr == attr)
            keys.emplace_back(key);
    }

    for (const auto &key : keys)
    {
        const Pos pos = CMap::toPos(key);
        const uint8_t x = pos.x;
        const uint8_t y = pos.y;
        ++count;
        const uint8_t tile = m_map.at(x, y);
        const TileDef &def = getTileDef(tile);
        if (def.type == TYPE_DIAMOND)
        {
            --m_diamonds;
            checkClosure();
        }
        m_map.set(x, y, TILES_BLANK);
        m_map.setAttr(x, y, 0);
        m_sfx.emplace_back(sfx_t{.x = x, .y = y, .sfxID = SFX_SPARKLE, .timeout = SFX_SPARKLE_TIMEOUT});
    }
    return count;
}

/**
 * @brief Add health to player
 *
 * @param hp negative value will be deducted. total hp will be constrained.
 */
void CGame::addHealth(const int hp)
{
    if (isClosure())
        return;
    const int hurtStage = m_gameStats->get(S_PLAYER_HURT); // provides invincibility frames
    const auto skill = m_gameStats->get(S_SKILL);
    if (hp > 0)
    {
        const int hpToken = hp / (1 + 1 * skill);
        m_health = std::min(m_health + hpToken, maxHealth());
    }
    else if (hp < 0 &&
             !m_gameStats->get(S_GOD_MODE_TIMER) &&
             hurtStage == HurtNone)
    {
        const int shields = std::min(m_gameStats->get(S_SHIELD), (int)MAX_SHIELDS);
        const float damageFactor = (MAX_FACTOR - shields) / (float)MAX_FACTOR;
        const int hpToken = (int)damageFactor * (hp * (1 + 1 * skill));
        m_health = std::max(m_health + hpToken, 0);
        playSound(SOUND_OUCHFAST);
    }
    checkClosure();
}

bool CGame::isLevelCompleted() const
{
    // check state of all bosses (if isGoal)
    for (const auto &boss : m_bosses)
    {
        if (boss.isGoal() && !boss.isDone())
            return false;
    }
    return !m_diamonds; // check goal count
}

/**
 * @brief Check if level closure conditions are meet
 *
 */
void CGame::checkClosure()
{
    bool doClosure = false;
    if (!isClosure() && !m_health) // player died
    {
        doClosure = true;
    }
    else if (!isClosure() && isLevelCompleted()) // !m_diamonds
    {
        const uint16_t exitKey = m_map.states().getU(POS_EXIT);
        if (exitKey != 0)
        {
            // Exit Notification Message
            m_events.emplace_back(EVENT_EXIT_OPENED);
            const bool revealExit = m_gameStats->get(S_REVEAL_EXIT) != 0;
            const Pos exitPos = CMap::toPos(exitKey);
            if (!revealExit)
            {
                m_map.set(exitPos.x, exitPos.y, TILES_DOORS_LEAF);
                m_gameStats->set(S_REVEAL_EXIT, 1);
                playSound(SOUND_BELLS1);
            }
            doClosure = m_player.pos() == exitPos;
        }
        else
        {
            doClosure = true;
        }
    }
    else if (!isClosure() && m_gameStats->get(S_CHUTE) != 0)
    {
        // fallen down a chute
        // save player position
        m_chuteTarget = m_player.pos();
        doClosure = true;
    }

    if (doClosure)
    {
        if (m_health)
            playSound(SOUND_EXIT);
        else
            playSound(SOUND_DEATH);
        m_gameStats->set(S_CLOSURE, 1);
        m_gameStats->set(S_CLOSURE_TIMER, CLOSURE_TIMER);
    }
}

/**
 * @brief Set Game mode
 *
 * @param mode
 *        possible values: MODE_LEVEL_INTRO, MODE_PLAY, MODE_GAME_OVER etc.
 */
void CGame::setMode(const GameMode mode)
{
    LOGI("set game mode: %d", mode);
    m_mode = mode;
}

/**
 * @brief return game mode
 *
 * @return int
 *         possible values: MODE_LEVEL_INTRO, MODE_PLAY, MODE_GAME_OVER etc.
 */
CGame::GameMode CGame::mode() const
{
    return m_mode;
}

/**
 * @brief check if the player hp is 0
 *
 * @return true
 * @return false
 */
bool CGame::isPlayerDead()
{
    return m_health == 0;
}

/**
 * @brief Reduce Player Life Count by 1
 *
 */
void CGame::killPlayer()
{
    m_lives = m_lives ? m_lives - 1 : 0;
}

/**
 * @brief Return true if lives is set to 0
 *
 * @return true
 * @return false
 */
bool CGame::isGameOver() const
{
    return m_lives == 0;
}

/**
 * @brief returns the score
 *
 * @return int
 */
int CGame::score() const
{
    return m_score;
}

/**
 * @brief Returns player live count
 *
 * @return int
 */
int CGame::lives() const
{
    return m_lives;
}

/**
 * @brief return player hp count
 *
 * @return int
 */
int CGame::health() const
{
    return m_health;
}

/**
 * @brief return player list of keys
 *
 * @return userKeys_t& tileID for each key
 */
CGame::userKeys_t &CGame::keys()
{
    return m_keys;
}

/**
 * @brief add points to score. automatically add bonus lives
 *
 * @param points
 */
void CGame::addPoints(const int points)
{
    m_score += points;
    if (m_score >= m_nextLife)
    {
        m_nextLife += calcScoreLife();
        addLife();
    }
}

/**
 * @brief add a new life to the player
 *
 */
void CGame::addLife()
{
    m_lives = std::min(m_lives + 1, static_cast<int>(MAX_LIVES));
    playSound(SOUND_POWERUP);
}

/**
 * @brief Is player in God Mode
 *
 * @return true
 * @return false
 */
bool CGame::isGodMode() const
{
    return m_gameStats->get(S_GOD_MODE_TIMER) != 0;
}

/**
 * @brief Current player speed
 *
 * @return time slice
 */

int CGame::playerSpeed() const
{
    return m_gameStats->get(S_EXTRA_SPEED_TIMER) ? FAST_PLAYER_SPEED : DEFAULT_PLAYER_SPEED;
}

/**
 * @brief Player has ExtraSpeed?
 *
 * @return true
 * @return false
 */
bool CGame::hasExtraSpeed() const
{
    return m_gameStats->get(S_EXTRA_SPEED_TIMER) != 0;
}

/**
 * @brief  get monster list
 *
 * @param monsters list of monsters
 * @param count count of monsters
 */
std::vector<CActor> &CGame::getMonsters()
{
    return m_monsters;
}

/**
 * @brief get a specific Monster
 *
 * @param i index
 * @return CActor&
 */
CActor &CGame::getMonster(int i)
{
    return m_monsters[i];
}

bool CGame::validateSignature(const char *signature, const uint32_t version)
{
    if (memcmp(signature, GAME_SIGNATURE, sizeof(GAME_SIGNATURE)) != 0)
    {
        char tmp[sizeof(GAME_SIGNATURE) + 1];
        memcpy(tmp, &signature, sizeof(GAME_SIGNATURE));
        tmp[sizeof(GAME_SIGNATURE)] = '\0';

        char gameSig[sizeof(GAME_SIGNATURE) + 1];
        memcpy(gameSig, GAME_SIGNATURE, sizeof(GAME_SIGNATURE));
        gameSig[sizeof(GAME_SIGNATURE)] = '\0';

        LOGW("savefile signature mismatch: `%s` -- expecting `%s`", signature, gameSig);
        return false;
    }
    if (version != ENGINE_VERSION)
    {
        LOGW("savegame version mismatched: 0x%.8x -- expecting 0x%.8x", version, ENGINE_VERSION);
        return false;
    }
    return true;
}

/**
 * @brief Deserializes the game state from disk
 *
 * @param sfile file handle
 * @return true
 * @return false
 */
bool CGame::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size)
    {
        return sfile.read(ptr, size) == 1;
    };

    // check signature/version
    uint32_t signature = 0;
    _R(&signature, sizeof(signature));
    uint32_t version = 0;
    _R(&version, sizeof(version));
    if (!validateSignature(reinterpret_cast<char *>(&signature), version))
        return false;

    // ptr
    uint32_t indexPtr = 0;
    _R(&indexPtr, sizeof(indexPtr));

    // general information
    _R(&m_lives, sizeof(m_lives));
    _R(&m_health, sizeof(m_health));
    _R(&m_level, sizeof(m_level));
    _R(&m_nextLife, sizeof(m_nextLife));
    _R(&m_diamonds, sizeof(m_diamonds));
    _R(m_keys.tiles, sizeof(m_keys.tiles));
    clearKeyIndicators();
    _R(&m_score, sizeof(m_score));
    if (!m_player.read(sfile))
    {
        LOGE("failed to read player object");
        return false;
    };
    if (!m_gameStats->read(sfile))
    {
        LOGE("failed to read gamestats");
        return false;
    };

    // reading map
    CMap &map = getMap();
    if (!map.read(sfile))
    {
        LOGE("failed to read map");
        return false;
    }

    // monsters
    uint32_t actorCount = 0;
    _R(&actorCount, sizeof(uint32_t));
    m_monsters.clear();
    m_monsters.resize(actorCount);
    for (size_t i = 0; i < actorCount; ++i)
    {
        if (!m_monsters[i].read(sfile))
        {
            LOGE("failed to read actor %lu of %u", i, actorCount);
            return false;
        }
    }
    rebuildMonsterGrid();

    // bosses
    uint32_t bossCount = 0;
    readfile(&bossCount, sizeof(uint32_t));
    m_bosses.clear();
    m_bosses.resize(bossCount);
    for (size_t i = 0; i < bossCount; ++i)
    {
        if (!m_bosses[i].read(sfile))
        {
            LOGE("failed to read boss %lu of %u", i, bossCount);
            return false;
        }
    }

    // used items
    m_usedItems.clear();
    size_t usedItemCount = 0;
    _R(&usedItemCount, sizeof(uint32_t));
    for (size_t i = 0; i < usedItemCount; ++i)
    {
        Pos pos;
        pos.read(sfile);
        m_usedItems.emplace_back(pos);
    }

    // load sfx
    m_sfx.clear();
    uint16_t sfxCount = 0;
    _R(&sfxCount, sizeof(sfxCount));
    for (uint16_t i = 0; i < sfxCount; ++i)
    {
        sfx_t sfx;
        _R(&sfx, sizeof(sfx));
        m_sfx.emplace_back(std::move(sfx));
    }

    // clear events
    clearEvents();

    // read animation state
    if (!m_animator->read(sfile))
        return false;

    return true;
}

/**
 * @brief Serializes the game state from disk
 *
 * @param sfile file handle
 * @return true
 * @return false
 */

bool CGame::write(IFile &tfile)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == 1;
    };

    // writing signature/version
    _W(&GAME_SIGNATURE, sizeof(GAME_SIGNATURE));
    uint32_t version = ENGINE_VERSION;
    _W(&version, sizeof(version));

    // ptr
    uint32_t indexPtr = 0;
    _W(&indexPtr, sizeof(indexPtr));

    // write general information
    _W(&m_lives, sizeof(m_lives));
    _W(&m_health, sizeof(m_health));
    _W(&m_level, sizeof(m_level));
    _W(&m_nextLife, sizeof(m_nextLife));
    _W(&m_diamonds, sizeof(m_diamonds));
    _W(m_keys.tiles, sizeof(m_keys.tiles));
    _W(&m_score, sizeof(m_score));
    if (!m_player.write(tfile))
    {
        LOGE("failed write player object");
        return false;
    }
    if (!m_gameStats->write(tfile))
    {
        LOGE("failed write gameStats");
        return false;
    }

    // saving map
    CMap &map = getMap();
    if (!map.write(tfile))
    {
        LOGE("failed to write map");
        return false;
    }

    // monsters
    size_t actorCount = m_monsters.size();
    _W(&actorCount, sizeof(uint32_t));
    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        if (!m_monsters[i].write(tfile))
        {
            LOGE("failed to write actor %lu of %lu", i, actorCount);
            return false;
        }
    }

    // bosses
    size_t bossCount = m_bosses.size();
    _W(&bossCount, sizeof(uint32_t));
    for (size_t i = 0; i < m_bosses.size(); ++i)
    {
        if (!m_bosses[i].write(tfile))
        {
            LOGE("failed to write boss %lu of %lu", i, bossCount);
            return false;
        }
    }

    // used items
    size_t usedItemCount = m_usedItems.size();
    _W(&usedItemCount, sizeof(uint32_t));
    for (const Pos &pos : m_usedItems)
        pos.write(tfile);

    // save sfx
    uint16_t sfxCount = (uint16_t)m_sfx.size();
    _W(&sfxCount, sizeof(sfxCount));
    for (const auto &sfx : m_sfx)
        _W(&sfx, sizeof(sfx));

    // save animation state
    if (!m_animator->write(tfile))
        return false;

    return true;
}

/**
 * @brief set lives count for the player
 *
 * @param lives
 */
void CGame::setLives(int lives)
{
    m_lives = lives;
}

/**
 * @brief play a given sounds
 *
 * @param id must be SOUND_XXXX value
 */
void CGame::playSound(const int id) const
{
    if (id != SOUND_NONE && m_sound != nullptr)
    {
        m_sound->play(id);
    }
}

/**
 * @brief Play the sound for a given tileID
 *
 * @param tileID
 */
void CGame::playTileSound(int tileID) const
{
    int snd = SOUND_NONE;
    if (tileID == TILES_CHEST || tileID == TILES_AMULET1)
    {
        snd = SOUND_COIN1;
    }
    else if (isFruit(tileID))
    {
        snd = SOUND_GRUUP;
    }
    else if (tileID == TILES_DIAMOND)
    {
        snd = SOUND_GOLD3;
    }
    playSound(snd);
}

/**
 * @brief attach a ISound Interface for the pupose of playing sounds
 *
 * @param s
 */
void CGame::attach(std::shared_ptr<ISound> &s)
{
    m_sound = s;
}

/**
 * @brief  return current skill Level
 *
 * @return uint8_t SKILL_EASY, SKILL_NORMAL or SKILL_HARD
 */
uint8_t CGame::skill() const
{
    return m_gameStats->get(S_SKILL);
}

/**
 * @brief set skill level
 *
 * @param v SKILL_EASY, SKILL_NORMAL or SKILL_HARD
 */
void CGame::setSkill(const uint8_t v)
{
    m_gameStats->set(S_SKILL, v);
    m_nextLife = calcScoreLife();
}

/**
 * @brief calculate the next score threshold for a bonus life
 *
 * @return int
 */
int CGame::calcScoreLife() const
{
    const auto skill = m_gameStats->get(S_SKILL);
    return SCORE_LIFE * (1 + skill);
}

/**
 * @brief return map count
 *
 * @return int
 */
int CGame::size() const
{
    return m_mapArch->size();
}

/**
 * @brief get text for the selected hint
 *
 * @return const char*
 */
const char *CGame::getHintText()
{
    return m_hints.size() ? m_hints[m_introHint].c_str() : "";
}

/**
 * @brief parse text lines into hint strings
 *
 * @param data
 */
void CGame::parseHints(const char *data)
{
    m_hints.clear();
    const auto lines = split(std::string(data), '\n');
    for (const auto &line : lines)
    {
        if (!line.empty())
            m_hints.emplace_back(line);
    }
}

/**
 * @brief  get first eventID
 *
 * @return int
 */
int CGame::getEvent()
{
    int event = EVENT_NONE;
    if (m_events.size() > 0)
    {
        event = m_events.at(0);
        m_events.erase(m_events.begin());
    }
    return event;
}

/**
 * @brief is this tile a fruit?
 *
 * @param tileID
 * @return true
 * @return false
 */
bool CGame::isFruit(const uint8_t tileID)
{
    return getTileDef(tileID).flags & FLAG_FRUIT;
}

/**
 * @brief is this tile a treasure
 *
 * @param tileID
 * @return true
 * @return false
 */
bool CGame::isBonusItem(const uint8_t tileID)
{
    return getTileDef(tileID).flags & FLAG_TREASURE;
}

/**
 * @brief current sugar count
 *
 * @return int
 */
int CGame::sugar() const
{
    return m_gameStats->get(S_SUGAR);
}

/**
 * @brief get a mapReport for the current map
 *
 * @return MapReport
 */
MapReport CGame::currentMapReport()
{
    return generateMapReport(m_map);
}

/**
 * @brief Generate a statistical report for the map
 *
 * @param report
 */

MapReport CGame::generateMapReport(CMap &map)
{
    MapReport report;
    std::unordered_map<uint8_t, int> tiles;
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            const auto &tile = map.at(x, y);
            tiles[tile] += 1;
        }
    }

    std::unordered_map<uint8_t, int> secrets;
    const attrMap_t &attrs = map.attrs();
    for (const auto &[k, v] : attrs)
    {
        if (RANGE(v, SECRET_ATTR_MIN, SECRET_ATTR_MAX))
            ++secrets[v];
    }
    report.bonuses = 0;
    report.fruits = 0;
    report.secrets = secrets.size();
    for (const auto [tile, count] : tiles)
    {
        const TileDef &def = getTileDef(tile);
        if (def.type != TYPE_PICKUP)
            continue;
        if (isFruit(tile))
            report.fruits += count;
        if (isBonusItem(tile))
            report.bonuses += count;
    }
    return report;
}

/**
 * @brief Get Special effects List
 *
 * @return std::vector<sfx_t>&
 */
std::vector<sfx_t> &CGame::getSfx()
{
    return m_sfx;
}

/**
 * @brief Purge Expired Special effects
 *
 */
void CGame::purgeSfx()
{
    m_sfx.erase(std::remove_if(m_sfx.begin(), m_sfx.end(), [](auto &sfx)
                               { --sfx.timeout; return sfx.timeout == 0; }),
                m_sfx.end());
}

CGameStats &CGame::stats()
{
    return *m_gameStats;
}

const CGameStats &CGame::statsConst() const
{
    return *m_gameStats;
}

bool CGame::isRageMode() const
{
    return m_gameStats->get(S_RAGE_TIMER) != 0;
}

CGame *CGame::getGame()
{
    static std::unique_ptr<CGame> instance(new CGame);
    return instance.get();
}

/**
 * @brief Determines if the level closure is initiated
 *
 * @return true
 * @return false
 */
bool CGame::isClosure() const
{
    return m_gameStats->get(S_CLOSURE) != 0;
}

/**
 * @brief Raw Closure Timer for the current level
 *
 * @return int
 */
int CGame::closusureTimer() const
{
    return m_gameStats->get(S_CLOSURE_TIMER);
}

/**
 * @brief Decrement Closure Timer
 *
 */
void CGame::decClosure()
{
    m_gameStats->dec(S_CLOSURE_TIMER);
}

/**
 * @brief Determines if the player is frozen
 *
 * @return true
 * @return false
 */
bool CGame::isFrozen() const
{
    return m_gameStats->get(S_FREEZE_TIMER) != 0;
}

/**
 * @brief Destroy Game Singleton
 *
 */
void CGame::destroy()
{
    getGame();
}

/**
 * @brief Calculate MaxHealth for the current Skill Context
 *
 * @return int
 */
int CGame::maxHealth() const
{
    const auto skill = m_gameStats->get(S_SKILL);
    return static_cast<int>(MAX_HEALTH) / (1 + 1 * skill);
}

/**
 * @brief return userID associated with player sprite character
 *
 * @return int
 */

int CGame::getUserID() const
{
    return m_gameStats->get(S_USER);
}

/**
 * @brief Set the UserID for the player sprite
 *
 * @param userID
 */
void CGame::setUserID(const int userID) const
{
    m_gameStats->set(S_USER, userID);
}

/**
 * @brief Retrieve Map Original state report
 *        (this is generated when the map is loaded)
 *
 * @return const MapReport&
 */
const MapReport &CGame::originalMapReport()
{
    return m_report;
}

/**
 * @brief retrieve level time counter (time taken)
 *
 * @return int
 */
int CGame::timeTaken()
{
    return m_gameStats->get(S_TIME_TAKEN);
}

/**
 * @brief Increase the level time counter (time taken)
 *
 */
void CGame::incTimeTaken()
{
    m_gameStats->inc(S_TIME_TAKEN);
}

void CGame::resetKeys()
{
    memset(&m_keys, '\0', sizeof(m_keys));
}

void CGame::decKeyIndicators()
{
    for (auto &u : m_keys.indicators)
        u ? --u : 0;
}

void CGame::clearKeyIndicators()
{
    memset(m_keys.indicators, '\0', sizeof(m_keys.indicators));
}

int CGame::defaultLives()
{
    return m_defaultLives;
}

void CGame::setDefaultLives(int lives)
{
    m_defaultLives = lives;
    m_lives = lives;
}

void CGame::setQuiet(bool state)
{
    m_quiet = state;
}

const std::vector<CBoss> &CGame::bosses()
{
    return m_bosses;
}

void CGame::deleteMonster(const int i)
{
    if (i < 0 || i >= (int)m_monsters.size())
        return;

    // Remove from vector
    m_monsters.erase(m_monsters.begin() + i);

    // Rebuild indices
    rebuildMonsterGrid();
}

Random &CGame::getRandom()
{
    return g_randomz;
}

bool CGame::isBulletType(const uint8_t typeID)
{
    return typeID == TYPE_FIREBALL || typeID == TYPE_LIGHTNING_BOLT;
}

bool CGame::isMoveableType(const uint8_t typeID)
{
    return typeID == TYPE_BOULDER || typeID == TYPE_ICECUBE;
}

bool CGame::isOneTimeItem(const uint8_t tileID)
{
    return getTileDef(tileID).flags & FLAG_ONE_TIME;
}

bool CGame::shadowActorMove(CActor &actor, const JoyAim aim)
{
    uint16_t oldKey = CMap::toKey(actor.x(), actor.y());
    auto it = m_monsterGrid.find(oldKey);
    int monsterIndex = INVALID;
    if (it != m_monsterGrid.end())
    {
        monsterIndex = it->second;
        m_monsterGrid.erase(it);
    }

    const Pos newPos = CGame::translate(actor.pos(), aim);
    if (monsterIndex != INVALID && m_map.isValid(newPos.x, newPos.y))
    {
        m_monsterGrid[CMap::toKey(newPos.x, newPos.y)] = monsterIndex;
        actor.move(aim);
        return true;
    }
    return false;
}

void CGame::rebuildMonsterGrid()
{
    m_monsterGrid.clear();
    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        const CActor &m = m_monsters[i];
        if (m_map.isValid(m.x(), m.y()))
            updateMonsterGrid(m, i);
    }
}

void CGame::updateMonsterGrid(const CActor &actor, const int monsterIndex)
{
    const Pos pos = actor.pos();
    if (monsterIndex != INVALID && m_map.isValid(pos.x, pos.y))
        m_monsterGrid[CMap::toKey(pos.x, pos.y)] = monsterIndex;
}

scan_t CGame::scanPos(const Pos &pos) const
{
    scan_t result{};
    const CMap &map = getMap();
    const int layerCount = (int)map.layerCount();
    for (int i = layerCount - 1; i >= 0; --i)
    {
        const CLayer *layer = map.getLayer(i);
        if (!layer)
            continue;

        const uint8_t tileID = layer->at(pos.x, pos.y);
        if (layer->baseID() == 0)
        {
            const TileDef &def = getTileDef(tileID);
            if (def.type == TileType::TYPE_SWAMP)
                result.isWater = true;
        }
        else
        {
            const auto curr = m_animator->getLayerTile(tileID);
            const layerdata_t &data = getLayerTileDef(curr);
            if (data.tileType == LayerTileType::Deadly)
                result.isDeadly = true;
            else if (data.tileType == LayerTileType::Solid)
                result.isSolid = true;
            else if (data.tileType == LayerTileType::Water)
                result.isWater = true;
        }
    }
    return result;
}

void CGame::clearEvents()
{
    m_events.clear();
}