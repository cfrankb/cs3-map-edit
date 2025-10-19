/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

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

#include "game.h"
#include <vector>
#include <queue>
#include "map.h"
#include "boss.h"
#include "attr.h"
#include "tilesdata.h"
#include "sprtypes.h"
#include "logger.h"
#include "actor.h"
#include "randomz.h"
#include "ai_path.h"
#include "sounds.h"

namespace Game
{
    enum
    {
        ICE_CUBE_DAMAGE = 16,
        CRUSHER_SPEED_MASK = 3,
        AUTOKILL = -1024,
    };
}

using namespace Game;

/////////////////////////////////////////////////////////////////////

CActor *CGame::spawnBullet(int x, int y, JoyAim aim, uint8_t tile)
{
    if (x < 0 || y < 0 || x >= m_map.len() || y >= m_map.hei())
    {
        LOGW("Cannot spawn at invalid coordinates: %d,%d", x, y);
        return nullptr;
    }

    const TileDef &def = getTileDef(tile);
    const uint8_t pu = m_map.at(x, y);
    CActor actor(x, y, def.type, aim);
    actor.setPU(pu);
    if (pu == TILES_BLANK)
    {
        m_map.set(x, y, tile);
        int size = addMonster(actor);
        return &m_monsters[size - 1];
    }
    return nullptr;
}

void CGame::handleBossPath(CBoss &boss)
{
    const int bx = boss.x() / 2;
    const int by = boss.y() / 2;
    const AStar aStar;
    const AStarSmooth aStarSmooth;
    const BFS bFS;
    const LineOfSight lineOfSight;
    const CActor &player = m_player;

    Pos playerPos{static_cast<int16_t>(m_player.x() * CBoss::BOSS_GRANULAR_FACTOR),
                  static_cast<int16_t>(m_player.y() * CBoss::BOSS_GRANULAR_FACTOR)};

    if (playerPos.x < 0 || playerPos.x >= m_map.len() * boss.getGranularFactor() ||
        playerPos.y < 0 || playerPos.y >= m_map.hei() * boss.getGranularFactor())
    {
        LOGE("Invalid player position (%d,%d) for map bounds (%d,%d)",
             playerPos.x, playerPos.y, m_map.len() * boss.getGranularFactor(),
             m_map.hei() * boss.getGranularFactor());
        return;
    }

    const uint8_t algo = boss.data()->path;
    if (algo == BossData::ASTAR)
    {
        if (boss.followPath(playerPos, aStar))
            return;
    }
    else if (algo == BossData::LOS)
    {
        if (boss.followPath(playerPos, lineOfSight))
            return;
    }
    else if (algo == BossData::BFS)
    {
        if (boss.followPath(playerPos, bFS))
            return;
    }
    else if (algo == BossData::ASTAR_SMOOTH)
    {
        if (boss.followPath(playerPos, aStarSmooth))
            return;
    }
    else
    {
        LOGE("unsupported ai algo: %u", algo);
    }

    // Fallback movement
    if (bx < player.x() && boss.canMove(JoyAim::AIM_RIGHT))
    {
        boss.move(JoyAim::AIM_RIGHT);
    }
    else if (bx > player.x() && boss.canMove(JoyAim::AIM_LEFT))
    {
        boss.move(JoyAim::AIM_LEFT);
    }
    else if (by < player.y() && boss.canMove(JoyAim::AIM_DOWN))
    {
        boss.move(JoyAim::AIM_DOWN);
    }
    else if (by > player.y() && boss.canMove(JoyAim::AIM_UP))
    {
        boss.move(JoyAim::AIM_UP);
    }
}

void CGame::handleBossBullet(CBoss &boss)
{
    const int bx = boss.x() / 2;
    const int by = boss.y() / 2;
    const CActor &player = m_player;
    CActor *bullet = nullptr;
    const auto tileID = boss.data()->bullet;

    if (player.y() < by - boss.hitbox().height)
    {
        bullet = spawnBullet(bx, by - boss.hitbox().height, JoyAim::AIM_UP, tileID);
    }
    else if (player.y() > by + boss.hitbox().height)
    {
        bullet = spawnBullet(bx, by + boss.hitbox().height, JoyAim::AIM_DOWN, tileID);
    }
    else if (player.x() < bx)
    {
        bullet = spawnBullet(bx - 1, by - 1, JoyAim::AIM_LEFT, tileID);
    }
    else if (player.x() > bx + boss.hitbox().width)
    {
        bullet = spawnBullet(bx + boss.hitbox().width, by - 1, JoyAim::AIM_RIGHT, tileID);
    }
    if (bullet != nullptr)
    {
        playSound(SOUND_SHOOT3);
        boss.setState(CBoss::BossState::Attack);
        playSound(SOUND_DINOSAUR3);
    }
}

void CGame::manageBosses(const int ticks)
{
    auto &rng = getRandom();
    rng.setTick(ticks);

    const CActor &player = m_player;
    for (auto &boss : m_bosses)
    {
        if (boss.state() == CBoss::BossState::Hidden)
            continue;

        // customize animation speed
        const int a_speed = boss.data()->a_speed;
        const uint32_t boss_flags = boss.data()->flags;
        if (a_speed == 0 || (ticks % a_speed) == 0)
            boss.animate();

        // ignore dead bosses
        if (boss.state() == CBoss::BossState::Death)
            continue;

        const int bx = boss.x() / 2;
        const int by = boss.y() / 2;
        if (boss.testHitbox(CBoss::isPlayer, nullptr))
            addHealth(-boss.damage());
        if (boss_flags & BOSS_FLAG_ICE_DAMAGE)
            if (boss.testHitbox(CBoss::isIceCube, CBoss::meltIceCube))
            {
                playSound(SOUND_SPLASH01);
                const bool justDied = boss.subtainDamage(ICE_CUBE_DAMAGE);
                if (justDied)
                    addPoints(boss.data()->score);
            }

        if (boss.speed() != 0 && (ticks % boss.speed() != 0))
            continue;

        if (boss.state() == CBoss::BossState::Patrol)
        {
            boss.patrol();
            if (boss.distance(player) <= boss.data()->chase_distance)
            {
                boss.setState(CBoss::BossState::Chase);
            }
        }
        else if (boss.state() == CBoss::BossState::Chase)
        {
            if (boss.distance(player) > boss.data()->pursuit_distance)
            {
                boss.setState(CBoss::BossState::Patrol);
                continue;
            }

            if (boss.data()->bullet != TILES_BLANK)
            {
                // Fireball spawning
                if (rng.range(0, boss.data()->bullet_speed) == 0 && bx > 2 && by > 0)
                {
                    handleBossBullet(boss);
                }
            }
            handleBossPath(boss);
        }
    }
}

/**
 * @brief Handle all the monsters on the current map
 *
 * @param ticks clock ticks since start
 */

void CGame::manageMonsters(const int ticks)
{
    std::vector<CActor> newMonsters;
    std::set<int, std::greater<int>> deletedMonsters;

    auto isDeleted = [&deletedMonsters](int i)
    {
        return deletedMonsters.find(i) != deletedMonsters.end();
    };

    constexpr int speedCount = 9;
    bool speeds[speedCount];
    for (uint32_t i = 0; i < sizeof(speeds); ++i)
    {
        speeds[i] = i ? (ticks % i) == 0 : true;
    }

    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        if (isDeleted(i))
            continue;
        CActor &actor = m_monsters[i];
        const Pos pos = actor.pos();
        const uint8_t cs = m_map.at(pos.x, pos.y);
        const uint8_t attr = m_map.getAttr(pos.x, pos.y);
        if (RANGE(attr, ATTR_WAIT_MIN, ATTR_WAIT_MAX))
        {
            const uint8_t distance = (attr & 0xf) + 1;
            if (actor.distance(m_player) <= distance)
                m_map.setAttr(pos.x, pos.y, 0);
            else
                continue;
        }

        const TileDef &def = getTileDef(cs);
        if (!speeds[def.speed])
        {
            continue;
        }
        if (actor.type() == TYPE_MONSTER)
        {
            handleMonster(actor, def);
        }
        else if (actor.type() == TYPE_DRONE)
        {
            handleDrone(actor, def);
        }
        else if (actor.type() == TYPE_VAMPLANT)
        {
            handleVamPlant(actor, def, newMonsters);
        }
        else if (RANGE(actor.type(), ATTR_CRUSHER_MIN, ATTR_CRUSHER_MAX))
        {
            handleCrusher(actor, speeds);
        }
        else if (actor.type() == TYPE_ICECUBE)
        {
            handleIceCube(actor);
        }
        else if (actor.type() == TYPE_FIREBALL)
        {
            handleFirball(actor, def, i, deletedMonsters);
        }
        else if (actor.type() == TYPE_BOULDER)
        {
            // Do nothing for now
        }
        else if (actor.type() == TYPE_LIGHTNING_BOLT)
        {
            handleLightningBolt(actor, def, i, deletedMonsters);
        }
        else
        {
            LOGW("unhandled monster type: %.2x at index %lu", actor.type(), i);
        }
    }

    // moved here to avoid reallocation while using a reference
    for (auto const &monster : newMonsters)
    {
        addMonster(monster);
    }

    // remove deleted monsters
    for (auto const &i : deletedMonsters)
    {
        m_monsters.erase(m_monsters.begin() + i);
    }
}

void CGame::handleMonster(CActor &actor, const TileDef &def)
{
    constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};

    if (actor.isPlayerThere(actor.getAim()))
    {
        // apply health damages
        addHealth(def.health);
        if (def.ai & AI_STICKY)
        {
            return;
        }
    }
    bool reverse = def.ai & AI_REVERSE;
    JoyAim aim = actor.findNextDir(reverse);
    if (aim != AIM_NONE)
    {
        actor.move(aim);
        if (!(def.ai & AI_ROUND))
        {
            return;
        }
    }
    for (uint8_t i = 0; i < sizeof(g_dirs); ++i)
    {
        if (actor.getAim() != g_dirs[i] &&
            actor.isPlayerThere(g_dirs[i]))
        {
            // apply health damages
            addHealth(def.health);
            if (def.ai & AI_FOCUS)
            {
                actor.setAim(g_dirs[i]);
            }
            break;
        }
    }
}

void CGame::handleDrone(CActor &actor, const TileDef &def)
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
        if (def.ai & AI_STICKY)
        {
            return;
        }
    }
    if (actor.canMove(aim))
    {
        actor.move(aim);
    }
    else if (aim == AIM_LEFT)
        aim = AIM_RIGHT;
    else
        aim = AIM_LEFT;
    actor.setAim(aim);
}

void CGame::handleVamPlant(CActor &actor, const TileDef &def, std::vector<CActor> &newMonsters)
{
    constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};

    for (uint8_t i = 0; i < sizeof(g_dirs); ++i)
    {
        const Pos p = CGame::translate(Pos{actor.x(), actor.y()}, g_dirs[i]);
        const uint8_t ct = m_map.at(p.x, p.y);
        const TileDef &defT = getTileDef(ct);
        if (defT.type == TYPE_PLAYER)
        {
            // apply damage from config
            addHealth(def.health);
        }
        else if (defT.type == TYPE_SWAMP)
        {
            m_map.set(p.x, p.y, TILES_VAMPLANT);
            newMonsters.emplace_back(CActor(p.x, p.y, TYPE_VAMPLANT));
            break;
        }
        else if (defT.type == TYPE_MONSTER || defT.type == TYPE_DRONE)
        {
            const int j = findMonsterAt(p.x, p.y);
            if (j == INVALID)
                continue;
            CActor &m = m_monsters[j];
            m.setType(TYPE_VAMPLANT);
            m_map.set(p.x, p.y, TILES_VAMPLANT);
            break;
        }
    }
}

void CGame::handleCrusher(CActor &actor, const bool speeds[])
{
    const uint8_t speed = (actor.type() & CRUSHER_SPEED_MASK) + SPEED_VERYFAST;
    if (!speeds[speed])
    {
        return;
    }
    JoyAim aim = actor.getAim();
    const bool isPlayerThere = actor.isPlayerThere(aim);
    if (isPlayerThere && !isGodMode())
    {
        // apply health damages
        addHealth(AUTOKILL);
    }
    if (actor.canMove(aim) && !(isPlayerThere && isGodMode()))
        actor.move(aim);
    else if (aim == AIM_LEFT)
        aim = AIM_RIGHT;
    else if (aim == AIM_RIGHT)
        aim = AIM_LEFT;
    else if (aim == AIM_DOWN)
        aim = AIM_UP;
    else if (aim == AIM_UP)
        aim = AIM_DOWN;
    actor.setAim(aim);
}

void CGame::handleIceCube(CActor &actor)
{
    JoyAim aim = actor.getAim();
    if (aim != JoyAim::AIM_NONE)
    {
        if (actor.canMove(aim))
            actor.move(aim);
        else
            actor.setAim(JoyAim::AIM_NONE);
    }
}

void CGame::handleFirball(CActor &actor, const TileDef &def, const int i, std::set<int, std::greater<int>> &deletedMonsters)
{
    JoyAim aim = actor.getAim();
    if (actor.canMove(aim))
    {
        actor.move(aim);
    }
    else
    {
        playSound(SOUND_HIT2);
        // remove actor/ set to be deleted
        m_map.set(actor.x(), actor.y(), actor.getPU());
        deletedMonsters.insert(i);
        m_sfx.emplace_back(sfx_t{.x = actor.x(), .y = actor.y(), .sfxID = SFX_EXPLOSION1, .timeout = SFX_EXPLOSION1_TIMEOUT});

        if (CGame::translate(Pos{actor.x(), actor.y()}, aim) == actor.pos())
            // coordonate outside map bounds
            return;
        const uint8_t tileID = actor.tileAt(aim);
        const TileDef &defX = getTileDef(tileID);
        if (defX.type == TYPE_ICECUBE)
        {
            playSound(SOUND_SPLASH01);
            const Pos &pos = translate(actor.pos(), aim);
            int i = findMonsterAt(pos.x, pos.y);
            if (i != INVALID)
            {
                deletedMonsters.insert(i);
                m_sfx.emplace_back(sfx_t{.x = pos.x, .y = pos.y, .sfxID = SFX_EXPLOSION6, .timeout = SFX_EXPLOSION6_TIMEOUT});
                m_map.set(pos.x, pos.y, TILES_BLANK);
            }
        }
        else if (actor.isPlayerThere(aim) && !isGodMode())
        {
            addHealth(def.health);
        }
    }
}

void CGame::handleLightningBolt(CActor &actor, const TileDef &def, const int i, std::set<int, std::greater<int>> &deletedMonsters)
{
    JoyAim aim = actor.getAim();
    if (actor.canMove(aim))
    {
        actor.move(aim);
    }
    else
    {
        playSound(SOUND_HIT2);
        // remove actor/ set to be deleted
        m_map.set(actor.x(), actor.y(), actor.getPU());
        deletedMonsters.insert(i);
        m_sfx.emplace_back(sfx_t{.x = actor.x(), .y = actor.y(), .sfxID = SFX_EXPLOSION1, .timeout = SFX_EXPLOSION1_TIMEOUT});

        const Pos &pos = translate(actor.pos(), aim);
        if (pos == actor.pos())
            // coordonate outside map bounds
            return;

        const uint8_t tileID = actor.tileAt(aim);
        const TileDef &defX = getTileDef(tileID);
        if (defX.type == TYPE_ICECUBE)
        {
            playSound(SOUND_SPLASH01);
            const Pos &pos = translate(actor.pos(), aim);
            int i = findMonsterAt(pos.x, pos.y);
            if (i != INVALID)
            {
                deletedMonsters.insert(i);
                m_sfx.emplace_back(sfx_t{.x = pos.x, .y = pos.y, .sfxID = SFX_EXPLOSION7, .timeout = SFX_EXPLOSION7_TIMEOUT});
                m_map.set(pos.x, pos.y, TILES_BLANK);
            }
        }
        else if (actor.isPlayerThere(aim) && !isGodMode())
        {
            addHealth(def.health);
        }
    }
}