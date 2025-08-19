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
#include <set>
#include <algorithm>
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

#ifdef USE_QFILE
#include <QDebug>
#define printf qDebug
#endif

CMap g_map(30, 30);
uint8_t CGame::m_keys[MAX_KEYS];
static constexpr const char GAME_SIGNATURE[]{'C', 'S', '3', 'b'};
CGame *g_game = nullptr;

/**
 * @brief Construct a new CGame::CGame object
 *
 */
CGame::CGame()
{
    printf("staring up version: 0x%.8x\n", VERSION);
    m_health = 0;
    m_level = 0;
    m_lives = DEFAULT_LIVES;
    m_score = 0;
    m_gameStats = new CGameStats;
    m_gameStats->set(S_SKILL, SKILL_EASY);
}

/**
 * @brief Destroy the CGame::CGame object
 *
 */
CGame::~CGame()
{
    if (m_sound)
    {
        delete m_sound;
    }

    if (m_gameStats)
    {
        delete m_gameStats;
    }
}

/**
 * @brief returns the current map
 *
 * @return CMap&
 */
CMap &CGame::getMap()
{
    return g_map;
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

/**
 * @brief move player in a give direction
 *
 * @param aim
 * @return true
 * @return false
 */
bool CGame::move(const JoyAim aim)
{
    if (m_player.canMove(aim))
    {
        m_player.move(aim);
        consume();
        return true;
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
    else if (def.type == TYPE_SWAMP)
    {
        addHealth(def.health);
    }

    if (isFruit(pu))
    {
        auto &sugar = m_gameStats->get(S_SUGAR);
        ++sugar;
        if (sugar != MAX_SUGAR_RUSH_LEVEL)
            m_events.push_back(EVENT_SUGAR);
    }

    // apply flags
    if (def.flags & FLAG_EXTRA_LIFE)
    {
        m_events.push_back(EVENT_EXTRA_LIFE);
        addLife();
    }

    if (def.flags & FLAG_GODMODE)
    {
        if (!m_gameStats->get(S_GOD_MODE_TIMER))
            playSound(SOUND_POWERUP3);
        m_gameStats->set(S_GOD_MODE_TIMER, GODMODE_TIMER);
        m_events.push_back(EVENT_GOD_MODE);
    }

    if (def.flags & FLAG_EXTRA_SPEED || m_gameStats->get(S_SUGAR) == MAX_SUGAR_RUSH_LEVEL)
    {
        if (!m_gameStats->get(S_EXTRA_SPEED_TIMER))
            playSound(SOUND_POWERUP2);
        m_gameStats->set(S_EXTRA_SPEED_TIMER, EXTRASPEED_TIMER);
        m_events.push_back(EVENT_SUGAR_RUSH);
        m_gameStats->set(S_SUGAR, 0);
    }

    if (def.flags & FLAG_RAGE)
    {
        if (!m_gameStats->get(S_RAGE_TIMER))
            playSound(SOUND_POWERUP3);
        m_gameStats->set(S_RAGE_TIMER, RAGE_TIMER);
        m_events.push_back(EVENT_RAGE);
    }

    // trigger key
    int x = m_player.getX();
    int y = m_player.getY();
    uint8_t attr = g_map.getAttr(x, y);
    if (attr != 0)
    {
        g_map.setAttr(x, y, 0);
        if (attr >= MSG0 && g_map.states().hasS(attr))
        {
            // Messsage Event (scrolls, books etc)
            m_events.push_back(attr);
        }
        else if (clearAttr(attr))
        {
            playSound(SOUND_0009);
            m_events.push_back(EVENT_SECRET);
        }
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
    printf("loading level: %d ...\n", m_level + 1);
    setMode(mode);

    // extract level from MapArch
    g_map = *(m_mapArch->at(m_level));

    printf("level loaded\n");
    if (m_hints.size() == 0)
    {
        printf("hints not loaded\n");
    }
    m_introHint = m_hints.size() ? rand() % m_hints.size() : 0;
    m_events.clear();

    // Use origin pos if available
    CStates &states = g_map.states();
    const uint16_t origin = states.getU(POS_ORIGIN);
    Pos pos;
    if (origin != 0)
    {
        pos = CMap::toPos(origin);
        g_map.set(pos.x, pos.y, TILES_ANNIE2);
    }
    else
    {
        pos = g_map.findFirst(TILES_ANNIE2);
    }
    printf("Player at: %d %d\n", pos.x, pos.y);
    m_player = CActor(pos, TYPE_PLAYER, AIM_DOWN);
    m_diamonds = states.hasU(MAP_GOAL) ? states.getU(MAP_GOAL) : g_map.count(TILES_DIAMOND);
    memset(m_keys, 0, sizeof(m_keys));
    m_health = DEFAULT_HEALTH;
    findMonsters();
    m_sfx.clear();
    resetStats();
    return true;
}

/**
 * @brief Increase Level by 1. Wraps back to 0.
 *
 */
void CGame::nextLevel()
{
    addPoints(LEVEL_BONUS + m_health);
    addPoints(g_map.states().getU(TIMEOUT) * 2);
    if (m_level != m_mapArch->size() - 1)
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
    m_events.clear();
    resetStats();
    loadLevel(MODE_RESTART);
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
    m_lives = DEFAULT_LIVES;
}

/**
 * @brief Decrenent Powerup timers
 *
 */
void CGame::decTimers()
{
    m_gameStats->dec(S_GOD_MODE_TIMER);
    m_gameStats->dec(S_EXTRA_SPEED_TIMER);
    m_gameStats->dec(S_RAGE_TIMER);
}

/**
 * @brief reset player state
 *
 */
void CGame::resetStats()
{
    m_gameStats->set(S_GOD_MODE_TIMER, 0);
    m_gameStats->set(S_EXTRA_SPEED_TIMER, 0);
    m_gameStats->set(S_RAGE_TIMER, 0);
    m_gameStats->set(S_SUGAR, 0);
    m_gameStats->set(S_CLOSURE, 0);
    m_gameStats->set(S_CLOSURE_TIMER, 0);
    m_gameStats->set(S_REVEAL_EXIT, 0);
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

/**
 * @brief Create a list of monster contained on the level
 *
 * @return true
 * @return false
 */
bool CGame::findMonsters()
{
    m_monsters.clear();
    for (int y = 0; y < g_map.hei(); ++y)
    {
        for (int x = 0; x < g_map.len(); ++x)
        {
            uint8_t c = g_map.at(x, y);
            const TileDef &def = getTileDef(c);
            if (def.type == TYPE_MONSTER ||
                def.type == TYPE_VAMPLANT ||
                def.type == TYPE_DRONE)
            {
                addMonster(CActor(x, y, def.type));
            }
        }
    }
    printf("%lu monsters found.\n", m_monsters.size());
    return true;
}

/**
 * @brief Add a monster to managed list
 *
 * @param actor
 * @return current monster count
 */
int CGame::addMonster(const CActor actor)
{
    m_monsters.push_back(actor);
    return (int)m_monsters.size();
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
    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        const CActor &actor = m_monsters[i];
        if (actor.getX() == x && actor.getY() == y)
        {
            return (int)i;
        }
    }
    return INVALID;
}

/**
 * @brief Handle all the monsters on the current map
 *
 * @param ticks clock ticks since start
 */

void CGame::manageMonsters(int ticks)
{
    const int speedCount = 9;
    bool speeds[speedCount];
    for (uint32_t i = 0; i < sizeof(speeds); ++i)
    {
        speeds[i] = i ? (ticks % i) == 0 : true;
    }

    const JoyAim dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
    std::vector<CActor> newMonsters;

    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        CActor &actor = m_monsters[i];
        const uint8_t cs = g_map.at(actor.getX(), actor.getY());
        const TileDef &def = getTileDef(cs);
        if (!speeds[def.speed])
        {
            continue;
        }
        if (def.type == TYPE_MONSTER)
        {
            if (actor.isPlayerThere(actor.getAim()))
            {
                // apply health damages
                addHealth(def.health);
                if (def.ai & AI_STICKY)
                {
                    continue;
                }
            }

            JoyAim aim = actor.findNextDir();
            if (aim != AIM_NONE)
            {
                actor.move(aim);
                if (!(def.ai & AI_ROUND))
                {
                    continue;
                }
            }
            for (uint8_t i = 0; i < sizeof(dirs); ++i)
            {
                if (actor.getAim() != dirs[i] &&
                    actor.isPlayerThere(dirs[i]))
                {
                    // apply health damages
                    addHealth(def.health);
                    if (def.ai & AI_FOCUS)
                    {
                        actor.setAim(dirs[i]);
                    }
                    break;
                }
            }
        }
        else if (def.type == TYPE_DRONE)
        {
            JoyAim aim = actor.getAim();
            if (aim < AIM_LEFT)
            {
                aim = AIM_LEFT;
            }
            if (actor.isPlayerThere(actor.getAim()))
            {
                // apply health damages
                addHealth(def.health);
            }
            if (actor.canMove(aim))
            {
                actor.move(aim);
            }
            else
            {
                aim ^= 1;
            }
            actor.setAim(aim);
        }
        else if (def.type == TYPE_VAMPLANT)
        {
            for (uint8_t i = 0; i < sizeof(dirs); ++i)
            {
                const Pos p = CGame::translate(Pos{actor.getX(), actor.getY()}, dirs[i]);
                const uint8_t ct = g_map.at(p.x, p.y);
                const TileDef &defT = getTileDef(ct);
                if (defT.type == TYPE_PLAYER)
                {
                    // apply damage from config
                    addHealth(def.health);
                }
                else if (defT.type == TYPE_SWAMP)
                {
                    g_map.set(p.x, p.y, TILES_VAMPLANT);
                    newMonsters.push_back(CActor(p.x, p.y, TYPE_VAMPLANT));
                    break;
                }
                else if (defT.type == TYPE_MONSTER)
                {
                    const int j = findMonsterAt(p.x, p.y);
                    if (j == INVALID)
                        continue;
                    CActor &m = m_monsters[j];
                    m.setType(TYPE_VAMPLANT);
                    g_map.set(p.x, p.y, TILES_VAMPLANT);
                    break;
                }
            }
        }
    }

    // moved here to avoid reallocation while using a reference
    for (auto const &monster : newMonsters)
    {
        addMonster(monster);
    }
}

uint8_t CGame::managePlayer(const uint8_t *joystate)
{
    decTimers();
    auto const pu = m_player.getPU();
    if (pu == TILES_SWAMP)
    {
        // apply health damage
        const TileDef &def = getTileDef(pu);
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
        if (t.y < g_map.hei() - 1)
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
        if (t.x < g_map.len() - 1)
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
    for (uint32_t i = 0; i < sizeof(m_keys); ++i)
    {
        if (m_keys[i] == c)
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
        if (m_keys[i] == c)
        {
            break;
        }
        if (m_keys[i] == '\0')
        {
            m_keys[i] = c;
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
    for (const auto &[key, tileAttr] : g_map.attrs())
    {
        if (tileAttr == attr)
            keys.push_back(key);
    }

    for (const auto &key : keys)
    {
        const Pos pos = CMap::toPos(key);
        const uint8_t x = pos.x;
        const uint8_t y = pos.y;
        ++count;
        const uint8_t tile = g_map.at(x, y);
        const TileDef &def = getTileDef(tile);
        if (def.type == TYPE_DIAMOND)
        {
            --m_diamonds;
            checkClosure();
        }
        g_map.set(x, y, TILES_BLANK);
        g_map.setAttr(x, y, 0);
        m_sfx.push_back(sfx_t{.x = x, .y = y, .sfxID = SFX_SPARKLE, .timeout = SFX_SPARKLE_TIMEOUT});
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

    const auto skill = m_gameStats->get(S_SKILL);
    if (hp > 0)
    {
        const int maxHealth = static_cast<int>(MAX_HEALTH) / (1 + 1 * skill);
        const int hpToken = hp / (1 + 1 * skill);
        m_health = std::min(m_health + hpToken, maxHealth);
    }
    else if (hp < 0 && !m_gameStats->get(S_GOD_MODE_TIMER))
    {
        const int hpToken = hp * (1 + 1 * skill);
        m_health = std::max(m_health + hpToken, 0);
        playSound(SOUND_OUCHFAST);
    }
    checkClosure();
}

void CGame::checkClosure()
{
    bool doClosure = false;
    if (!isClosure() && !m_health)
    {
        doClosure = true;
    }
    else if (!isClosure() && !m_diamonds)
    {
        const uint16_t exitKey = g_map.states().getU(POS_EXIT);
        if (exitKey != 0)
        {
            const bool revealExit = m_gameStats->get(S_REVEAL_EXIT) != 0;
            const Pos exitPos = CMap::toPos(exitKey);
            if (!revealExit)
            {
                g_map.set(exitPos.x, exitPos.y, TILES_DOORS_LEAF);
                m_gameStats->set(S_REVEAL_EXIT, 1);
            }
            doClosure = m_player.pos() == exitPos;
        }
        else
        {
            doClosure = true;
        }
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
 *        possible value: MODE_LEVEL_INTRO, MODE_PLAY, MODE_GAME_OVER etc.
 */
void CGame::setMode(const GameMode mode)
{
    m_mode = mode;
}

/**
 * @brief return game mode
 *
 * @return int
 *         possible value: MODE_LEVEL_INTRO, MODE_PLAY, MODE_GAME_OVER etc.
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
 * @return uint8_t* tileID for each key
 */
uint8_t *CGame::keys()
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

/**
 * @brief Deserializes the game state from disk
 *
 * @param sfile file handle
 * @return true
 * @return false
 */
bool CGame::read(FILE *sfile)
{
    auto readfile = [sfile](auto ptr, auto size)
    {
        return fread(ptr, size, 1, sfile) == 1;
    };

    // check signature/version
    uint32_t signature = 0;
    readfile(&signature, sizeof(signature));
    uint32_t version = 0;
    readfile(&version, sizeof(version));
    if (memcmp(GAME_SIGNATURE, &signature, sizeof(GAME_SIGNATURE)) != 0)
    {
        char sig[5] = {0, 0, 0, 0, 0};
        memcpy(sig, &signature, sizeof(signature));
        printf("savegame signature mismatched: %s\n", sig);
        return false;
    }
    if (version != VERSION)
    {
        printf("savegame version mismatched: 0x%.8x\n", version);
        return false;
    }

    // ptr
    uint32_t indexPtr = 0;
    readfile(&indexPtr, sizeof(indexPtr));

    // general information
    readfile(&m_lives, sizeof(m_lives));
    readfile(&m_health, sizeof(m_health));
    readfile(&m_level, sizeof(m_level));
    readfile(&m_nextLife, sizeof(m_nextLife));
    readfile(&m_diamonds, sizeof(m_diamonds));
    readfile(m_keys, sizeof(m_keys));
    readfile(&m_score, sizeof(m_score));
    m_player.read(sfile);
    m_gameStats->read(sfile);

    // reading map
    CMap &map = getMap();
    if (!map.read(sfile))
    {
        return false;
    }

    // monsters
    uint32_t count = 0;
    readfile(&count, sizeof(uint32_t));
    m_monsters.clear();
    for (size_t i = 0; i < count; ++i)
    {
        CActor tmp;
        tmp.read(sfile);
        m_monsters.push_back(tmp);
    }
    m_events.clear();
    m_sfx.clear();
    return true;
}

/**
 * @brief Serializes the game state from disk
 *
 * @param sfile file handle
 * @return true
 * @return false
 */

bool CGame::write(FILE *tfile)
{
    auto writefile = [tfile](auto ptr, auto size)
    {
        return fwrite(ptr, size, 1, tfile) == 1;
    };

    // writing signature/version
    writefile(&GAME_SIGNATURE, sizeof(GAME_SIGNATURE));
    uint32_t version = VERSION;
    writefile(&version, sizeof(version));

    // ptr
    uint32_t indexPtr = 0;
    writefile(&indexPtr, sizeof(indexPtr));

    // write general information
    writefile(&m_lives, sizeof(m_lives));
    writefile(&m_health, sizeof(m_health));
    writefile(&m_level, sizeof(m_level));
    writefile(&m_nextLife, sizeof(m_nextLife));
    writefile(&m_diamonds, sizeof(m_diamonds));
    writefile(m_keys, sizeof(m_keys));
    writefile(&m_score, sizeof(m_score));
    m_player.write(tfile);
    m_gameStats->write(tfile);

    // saving map
    CMap &map = getMap();
    map.write(tfile);

    // monsters
    uint32_t count = m_monsters.size();
    writefile(&count, sizeof(count));
    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        m_monsters[i].write(tfile);
    }
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
void CGame::attach(ISound *s)
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
    char *t = new char[strlen(data) + 1];
    strcpy(t, data);
    char *p = t;
    while (p && *p)
    {
        char *en = strstr(p, "\n");
        if (en)
        {
            *en = 0;
        }
        char *er = strstr(p, "\r");
        if (er)
        {
            *er = 0;
        }
        char *e = er > en ? er : en;
        while (*p == ' ' || *p == '\t')
        {
            ++p;
        }
        char *c = strstr(p, "#");
        if (c)
        {
            *c = '\0';
        }
        int i = strlen(p) - 1;
        while (i >= 0 && (p[i] == ' ' || p[i] == '\t'))
        {
            p[i] = '\0';
            --i;
        }
        if (p[0])
        {
            m_hints.push_back(p);
        }
        p = e ? e + 1 : nullptr;
    }
    delete[] t;
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
bool CGame::isFruit(const uint8_t tileID) const
{
    const uint8_t fruits[] = {
        TILES_APPLE,
        TILES_FRUIT1,
        TILES_WATERMELON,
        TILES_PEAR,
        TILES_CHERRY,
        TILES_STRAWBERRY,
        TILES_KIWI,
        TILES_JELLYJAR,
    };
    for (const auto &fruit : fruits)
    {
        if (tileID == fruit)
            return true;
    }
    return false;
}

bool CGame::isBonusItem(const uint8_t tileID) const
{
    const uint8_t tresures[] = {
        TILES_AMULET1,
        TILES_CHEST,
        TILES_GIFTBOX,
        TILES_LIGHTBUL,
        TILES_SCROLL1,
        TILES_SHIELD,
        TILES_CLOVER,
        TILES_1ST_AID,
        TILES_POTION1,
        TILES_POTION2,
        TILES_POTION3,
        TILES_FLOWERS,
        TILES_FLOWERS_2,
        TILES_TRIFORCE,
        TILES_ORB,
        TILES_TNTSTICK,
        TILES_SMALL_MUSH0,
        TILES_SMALL_MUSH1,
        TILES_SMALL_MUSH2,
        TILES_SMALL_MUSH3,
        TILES_REDBOOK,
    };
    for (const auto &tresure : tresures)
    {
        if (tileID == tresure)
            return true;
    }
    return false;
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
 * @brief Generate a statistical report for the map
 *
 * @param report
 */

void CGame::generateMapReport(MapReport &report)
{
    std::unordered_map<uint8_t, int> tiles;
    for (int y = 0; y < g_map.hei(); ++y)
    {
        for (int x = 0; x < g_map.len(); ++x)
        {
            const auto &tile = g_map.at(x, y);
            tiles[tile] += 1;
        }
    }

    std::unordered_map<uint8_t, int> secrets;
    const AttrMap &attrs = g_map.attrs();
    for (const auto &[k, v] : attrs)
    {
        if (v >= SECRET_ATTR_MIN &&
            v <= SECRET_ATTR_MAX)
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
}

std::vector<sfx_t> &CGame::getSfx()
{
    return m_sfx;
}

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

bool CGame::isRageMode() const
{
    return m_gameStats->get(S_RAGE_TIMER) != 0;
}

CGame *CGame::getGame()
{
    if (!g_game)
        g_game = new CGame;
    return g_game;
}

bool CGame::isClosure() const
{
    return m_gameStats->get(S_CLOSURE) != 0;
}

int CGame::closusureTimer() const
{
    return m_gameStats->get(S_CLOSURE_TIMER);
}

void CGame::decClosure()
{
    m_gameStats->dec(S_CLOSURE_TIMER);
}