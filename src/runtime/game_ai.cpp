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
#include "gamestats.h"
#include "tilesdefs.h"
#include "gamesfx.h"

namespace GamePrivate
{
    enum
    {
        ICE_CUBE_DAMAGE = 16,
        CRUSHER_SPEED_MASK = 3,
        AUTOKILL = -1024,
    };
}

using namespace GamePrivate;

/////////////////////////////////////////////////////////////////////

CActor *CGame::spawnBullet(int x, int y, JoyAim aim, uint8_t tile)
{
    if (x < 0 || y < 0 || x >= m_map.width() || y >= m_map.height())
    {
        LOGW("Cannot spawn at invalid coordinates: %d,%d", x, y);
        return nullptr;
    }

    const TileDef &def = getTileDef(tile);
    const uint8_t pu = m_map.at(x, y);
    CActor actor(x, y, def.type, aim);
    actor.setPU(pu);
    const TileDef &defPU = getTileDef(pu);
    if (defPU.type == TYPE_BACKGROUND || defPU.type == TYPE_STOP)
    {
        m_map.set(x, y, tile);
        m_monsters.emplace_back(std::move(actor));
        int index = m_monsters.size() - 1;
        updateMonsterGrid(m_monsters[index], index);
        return &m_monsters[index];
    }
    return nullptr;
}

void CGame::handleBossPath(CBoss &boss)
{
    const int bx = boss.x() / 2;
    const int by = boss.y() / 2;
    const CActor &player = m_player;

    Pos playerPos{static_cast<int16_t>(m_player.x() * CBoss::BOSS_GRANULAR_FACTOR),
                  static_cast<int16_t>(m_player.y() * CBoss::BOSS_GRANULAR_FACTOR)};

    if (playerPos.x < 0 || playerPos.x >= m_map.width() * boss.getGranularFactor() ||
        playerPos.y < 0 || playerPos.y >= m_map.height() * boss.getGranularFactor())
    {
        LOGE("Invalid player position (%d,%d) for map bounds (%d,%d)",
             playerPos.x, playerPos.y, m_map.width() * boss.getGranularFactor(),
             m_map.height() * boss.getGranularFactor());
        return;
    }

    const uint8_t algo = boss.data()->path;
    if (algo != BossData::Path::NONE)
    {
        auto pathAI = CPath::getPathAlgo(algo);
        if (pathAI)
            boss.followPath(playerPos, *pathAI);
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

bool CGame::handleBossBullet(CBoss &boss)
{
    const int bx = boss.x() / 2;
    const int by = boss.y() / 2;
    const CActor &player = m_player;
    CActor *bullet = nullptr;
    const auto tileID = boss.data()->bullet;
    const auto &hitbox = boss.hitbox();

    if (player.y() < by - hitbox.height)
    {
        bullet = spawnBullet(bx, by - hitbox.height, JoyAim::AIM_UP, tileID);
    }
    else if (player.y() > by + hitbox.height)
    {
        bullet = spawnBullet(bx, by + hitbox.height, JoyAim::AIM_DOWN, tileID);
    }
    else if (player.x() < bx)
    {
        bullet = spawnBullet(bx - 1, by - 1, JoyAim::AIM_LEFT, tileID);
    }
    else if (player.x() > bx + hitbox.width)
    {
        bullet = spawnBullet(bx + hitbox.width, by - 1, JoyAim::AIM_RIGHT, tileID);
    }
    if (bullet != nullptr)
    {
        playSound(boss.data()->bullet_sound);
        boss.setState(CBoss::BossState::Attack);
        playSound(boss.data()->attack_sound);
        if (boss.data()->bullet_algo != BossData::Path::NONE)
        {
            LOGI("bullet path algo: %d", boss.data()->bullet_algo);
            bullet->startPath(m_player.pos(), boss.data()->bullet_algo, boss.data()->bullet_ttl);
        }

        return true;
    }
    return false;
}

void CGame::handleBossHitboxContact(CBoss &boss)
{
    int playerDamage = 0;

    //  Attack hitbox: Affect all player tiles it overlaps
    boss.testHitbox1(m_map, [](const Pos &p, auto type)
                     {
                         (void)type;
                         return m_map.at(p.x, p.y) == TILES_ANNIE2; // check if player is there
                     },
                     [boss, this, &playerDamage](const HitResult &r)
                     {
                         playerDamage = std::max(boss.damage(r.type), playerDamage); // find player damage
                     });

    if (playerDamage)
        addHealth(-playerDamage); // hurtPlayer

    const uint32_t boss_flags = boss.data()->flags;
    if (boss_flags & BOSS_FLAG_ICE_DAMAGE)
    {
        boss.testHitbox1(m_map, [](const Pos &pos, auto type)
                         {
                             (void)type;
                             const CMap &map = CGame::getMap();
                             const auto c = map.at(pos.x, pos.y);
                             const TileDef &def = getTileDef(c);
                             return def.type == TYPE_ICECUBE; // check if IceCube
                         },
                         [&boss, this](const HitResult &r)
                         {
                             const Pos &pos = r.pos;
                             playSound(SOUND_SPLASH01);
                             if (r.type != BossData::HitBoxType::SPECIAL1)
                             {
                                 const bool justDied = boss.subtainDamage(ICE_CUBE_DAMAGE);
                                 if (justDied)
                                     addPoints(boss.data()->score);
                             }

                             // meltIceCube
                             CGame *game = CGame::getGame();
                             CMap &map = CGame::getMap();
                             int i = game->findMonsterAt(pos.x, pos.y);
                             if (i != CGame::INVALID)
                             {
                                 game->deleteMonster(i);
                                 game->getSfx().emplace_back(sfx_t{pos.x, pos.y, SFX_EXPLOSION6, SFX_EXPLOSION6_TIMEOUT});
                                 map.set(pos.x, pos.y, TILES_BLANK);
                             } //
                         });
    }

    // test if boss has set off barrel
    boss.testHitbox1(m_map, [](const Pos &pos, auto type)
                     {
                         if (type != BossData::HitBoxType::SPECIAL1)
                             return false;
                         const CMap &map = CGame::getMap();
                         const auto c = map.at(pos.x, pos.y);
                         const TileDef &def = getTileDef(c);
                         return def.type == TYPE_BARREL; // check if barrel
                     },
                     [&boss, this](const HitResult &r)
                     {
                         fuseBarrel(r.pos); // lit fuse for barrel
                     });
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
        const int speed_anime = boss.data()->speed_anime;
        const uint32_t boss_flags = boss.data()->flags;
        if (speed_anime == 0 || (ticks % speed_anime) == 0)
            boss.animate();

        // ignore dead bosses
        if (boss.state() == CBoss::BossState::Death)
            continue;

        const int bx = boss.x() / 2;
        const int by = boss.y() / 2;

        // hitbox attack
        handleBossHitboxContact(boss);

        if (boss.speed() != 0 && (ticks % boss.speed() != 0))
            continue;

        if (boss.state() == CBoss::BossState::Patrol)
        {
            boss.patrol();
            if (boss.distance(player) <= boss.data()->distance_chase)
            {
                boss.setState(CBoss::BossState::Chase);
            }
        }
        else if (boss.state() == CBoss::BossState::Chase)
        {
            if (boss.distance(player) > boss.data()->distance_pursuit)
            {
                boss.setState(CBoss::BossState::Patrol);
                continue;
            }

            if (boss.distance(player) <= boss.data()->distance_attack &&
                boss.data()->bullet != TILES_BLANK)
            {
                // Fireball spawning
                if (rng.range(0, boss.data()->bullet_rate) == 0 && bx > 2 && by > 0)
                {
                    handleBossBullet(boss);
                }
            }
            handleBossPath(boss);

            if ((boss_flags & BOSS_FLAG_PROXIMITY_ATTACK) &&
                boss.state() != CBoss::BossState::Attack &&
                (boss.distance(player) <= boss.data()->distance_attack) &&
                rng.range(0, boss.data()->bullet_rate) == 0)
            {
                // NEW: Compute direction to player
                const Pos boss_pos = boss.worldPos(); // Full-tile position (m_x/2, m_y/2)
                const Pos player_pos = player.pos();

                const int deltaX = player_pos.x - boss_pos.x;
                const int deltaY = player_pos.y - boss_pos.y;

                // Determine facing direction
                JoyAim facing = boss.getAim();
                if (abs(deltaX) > abs(deltaY))
                {
                    // Horizontal priority
                    facing = (deltaX > 0) ? JoyAim::AIM_RIGHT : JoyAim::AIM_LEFT;
                }
                else
                {
                    // Vertical priority
                    facing = (deltaY > 0) ? JoyAim::AIM_DOWN : JoyAim::AIM_UP;
                }
                boss.setAim(facing);
                boss.setState(CBoss::BossState::Attack);
            }
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
        speeds[i] = i ? (ticks % i) == 0 : true;

    for (size_t i = 0; i < m_monsters.size(); ++i)
    {
        if (isDeleted(i))
            continue;
        CActor &actor = m_monsters[i];
        const Pos pos = actor.pos();
        const uint8_t tileID = m_map.at(pos.x, pos.y);
        const uint8_t attr = m_map.getAttr(pos.x, pos.y);
        if (RANGE(attr, ATTR_IDLE_MIN, ATTR_IDLE_MAX))
        {
            const uint8_t distance = (attr & 0xf) + 1;
            if (actor.distance(m_player) <= distance)
                m_map.setAttr(pos.x, pos.y, 0);
            else
                continue;
        }

        const TileDef &def = getTileDef(tileID);
        if (!speeds[def.speed])
            continue;

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
            handleBullet(actor, def, i, {.sound = SOUND_HIT2, .sfxID = SFX_EXPLOSION1, .sfxTimeOut = SFX_EXPLOSION1_TIMEOUT}, deletedMonsters);
        }
        else if (actor.type() == TYPE_BOULDER)
        {
            // Do nothing for now
        }
        else if (actor.type() == TYPE_LIGHTNING_BOLT)
        {
            handleBullet(actor, def, i, {.sound = SOUND_HIT2, .sfxID = SFX_EXPLOSION7, .sfxTimeOut = SFX_EXPLOSION7_TIMEOUT}, deletedMonsters);
        }
        else if (actor.type() == TYPE_BARREL)
        {
            handleBarrel(actor, def, i, deletedMonsters);
        }
        else
        {
            LOGW("unhandled monster type: %.2x at index %lu", actor.type(), i);
        }
    }

    // moved here to avoid reallocation while using a reference
    for (auto &monster : newMonsters)
    {
        m_monsters.emplace_back(std::move(monster));
        int index = m_monsters.size() - 1;
        updateMonsterGrid(m_monsters[index], index); // Safe
    }

    // remove deleted monsters
    for (auto const &i : deletedMonsters)
    {
        m_monsters.erase(m_monsters.begin() + i);
    }

    // rebuild monster grid if needed
    if (!deletedMonsters.empty())
        rebuildMonsterGrid();
}

void CGame::handleMonster(CActor &actor, const TileDef &def)
{
    static constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
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
        shadowActorMove(actor, aim);
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
        shadowActorMove(actor, aim);
    }
    else if (aim == AIM_LEFT)
        aim = AIM_RIGHT;
    else
        aim = AIM_LEFT;
    actor.setAim(aim);
}

void CGame::handleVamPlant(CActor &actor, const TileDef &def, std::vector<CActor> &newMonsters)
{
    static constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};

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
        shadowActorMove(actor, aim);
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
    if (aim == AIM_NONE)
        return;

    Pos nextPos = translate(actor.pos(), aim);
    if (!m_map.isValid(nextPos.x, nextPos.y))
    {
        actor.setAim(AIM_NONE);
        return;
    }

    // Check player
    if (actor.isPlayerThere(aim) && !isGodMode())
    {
        addHealth(AUTOKILL);
        actor.setAim(AIM_NONE);
        return;
    }

    int targetIdx = findMonsterAt(nextPos.x, nextPos.y);
    if (targetIdx != INVALID)
    {
        if (!pushChain(nextPos.x, nextPos.y, aim))
        {
            actor.setAim(AIM_NONE);
            return;
        }
    }
    else if (!actor.canMove(aim))
    {
        actor.setAim(AIM_NONE);
        return;
    }

    // Move one tile
    shadowActorMove(actor, aim);
}

void CGame::blastRadius(const Pos &pos, const size_t radius, const int damage, std::set<int, std::greater<int>> &deletedMonsters)
{
    // compute blast radius
    std::vector<int> index;
    index.reserve(radius * 2 + 1);
    for (size_t i = 0; i < radius; ++i)
        index.emplace_back(-radius + i);
    index.emplace_back(0);
    for (size_t i = 0; i < radius; ++i)
        index.emplace_back(i + 1);

    struct blastPos_t
    {
        Pos pos;
        int damage;
        const Pos &toPos() const { return pos; }
    };
    std::vector<blastPos_t> blastPositions;

    // apply blast radius
    for (size_t i = 0; i < index.size(); ++i)
    {
        const auto &ty = index[i];
        for (size_t j = 0; j < index.size(); ++j)
        {
            const auto &tx = index[j];
            const int16_t x = pos.x + tx;
            const int16_t y = pos.y + ty;
            if (m_map.isValid(x, y))
            {
                int distance = (std::abs(tx) + std::abs(ty)) / 2;
                int radiusDamage = distance ? damage / std::abs(distance) : damage;
                blastPositions.emplace_back(blastPos_t{Pos{x, y}, std::abs(radiusDamage)});

                // check player for splash danage
                if (m_player.pos() == Pos{x, y})
                {
                    addHealth(radiusDamage);
                    continue;
                }

                if (tx == 0 && ty == 0)
                    continue;

                // check for intersection with mob monster and other actors
                const int id = findMonsterAt(x, y);
                if (id != INVALID && !deletedMonsters.count(id))
                {
                    CActor &actor = m_monsters[id];
                    if (actor.type() == TYPE_BARREL)
                    {
                        // light other barrels
                        fuseBarrel({x, y});
                    }
                    else if (actor.type() == TYPE_MONSTER || actor.type() == TYPE_DRONE || actor.type() == TYPE_VAMPLANT)
                    {
                        // kill mob monsters
                        deletedMonsters.emplace(id);
                        m_sfx.emplace_back(sfx_t{pos.x, pos.y, SFX_EXPLOSION0, SFX_EXPLOSION0_TIMEOUT});
                    }
                    else if (actor.type() == TYPE_ICECUBE)
                    {
                        // melt icecubes
                        deletedMonsters.emplace(id);
                        m_sfx.emplace_back(sfx_t{pos.x, pos.y, SFX_EXPLOSION6, SFX_EXPLOSION6_TIMEOUT});
                    }
                }
            }
        }
    }

    // test boss hitbox
    for (auto &boss : m_bosses)
    {
        int bossDamage = 0;
        for (const auto &bp : blastPositions)
        {
            boss.testHitbox1(m_map, [&bp](const Pos &p, const auto type) { //
                // check if damage can occur
                return type != BossData::HitBoxType::SPECIAL1 && p == bp.toPos();
            },
                             [boss, &bp, &bossDamage](const HitResult &)
                             {
                                 // compute maximum damage
                                 bossDamage = std::max(bp.damage, bossDamage); //
                             });
        }
        if (bossDamage)
            boss.subtainDamage(bossDamage);
    }
}

void CGame::handleBarrel(CActor &actor, const TileDef &def, const int i, std::set<int, std::greater<int>> &deletedMonsters)
{
    if (actor.decTTL() == 0)
    {
        // if barrel is exploding
        const Pos pos = actor.pos();
        m_sfx.emplace_back(sfx_t{
            .x = pos.x,
            .y = pos.y,
            .sfxID = SFX_EXPLOSION5,
            .timeout = SFX_EXPLOSION5_TIMEOUT,
        });
        deletedMonsters.emplace(i);
        m_map.set(pos.x, pos.y, TILES_BARREL2EX);
        playSound(SOUND_EXPLOSION1);
        m_gameStats->set(S_FLASH, 1);
        blastRadius(pos, 2, def.health, deletedMonsters);
    }
}

void CGame::handleBullet(CActor &actor, const TileDef &def, const int i, const bulletData_t &bullet, std::set<int, std::greater<int>> &deletedMonsters)
{
    bool isMoving;
    JoyAim aim = actor.getAim();
    if (actor.isFollowingPath())
    {
        auto result = actor.followPath(m_player.pos());
        if (result == CPath::Result::MoveSuccesful)
            return;
        isMoving = result != CPath::Result::Blocked && actor.getTTL() != 0;
        aim = actor.getAim();
        // if (actor.canMove(aim) && actor.getTTL() != 0)
        //     return;
        // LOGI("sprite: %p not moving result:[%d]; aim=[%d] isPlayerThere=[%d] [%p]", &actor, result, actor.getAim(), actor.isPlayerThere(aim), actor.path());
    }
    else
    {
        isMoving = actor.canMove(aim);
        if (isMoving)
            shadowActorMove(actor, aim);
    }

    if (!isMoving || (actor.getTTL() == 0 && actor.algo() != BossData::Path::NONE))
    {
        playSound(bullet.sound);
        // remove actor/ set to be deleted
        m_map.set(actor.x(), actor.y(), actor.getPU());
        deletedMonsters.insert(i);
        m_sfx.emplace_back(sfx_t{
            .x = actor.x(),
            .y = actor.y(),
            .sfxID = bullet.sfxID,
            .timeout = bullet.sfxTimeOut,
        });
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
        else if (defX.type == TYPE_BARREL)
        {
            const Pos &pos = translate(actor.pos(), aim);
            fuseBarrel(pos);
        }
    }
}

bool CGame::isPushable(const uint8_t typeID)
{
    return typeID == TYPE_BOULDER || typeID == TYPE_ICECUBE;
}

bool CGame::pushChain(const int x, const int y, const JoyAim aim)
{
    if (!m_map.isValid(x, y))
        return false;

    int i = findMonsterAt(x, y);
    if (i == INVALID)
        return true;
    if (!isPushable(m_monsters[i].type()))
        return false;

    // CRITICAL: Check canMove() BEFORE pushing
    if (!m_monsters[i].canMove(aim))
        return false;

    Pos next = translate({(int16_t)x, (int16_t)y}, aim);
    if (next.x == x && next.y == y)
        return false; // translate failed
    if (!m_map.isValid(next.x, next.y))
        return false; // out of bounds

    if (!pushChain(next.x, next.y, aim))
        return false;

    m_monsters[i].setAim(aim);
    shadowActorMove(m_monsters[i], aim);
    return true;
}
