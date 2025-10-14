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
#include <cmath>
#include <memory>
#include <algorithm>
#include <set>
#include "map.h"
#include "boss.h"
#include "attr.h"
#include "tilesdata.h"
#include "sprtypes.h"
#include "logger.h"
#include "actor.h"

constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
constexpr int CHASE_DISTANCE = 10;
constexpr int TARGET_DISTANCE = 5;

// Represents a grid cell (node) in the A* algorithm
struct Node
{
    // int x, y;     // Coordinates
    Pos pos;
    int gCost;    // Cost from start to this node
    int hCost;    // Heuristic (estimated cost to goal)
    Node *parent; // Parent node for path reconstruction

    Node(Pos pos, int g = 0, int h = 0, Node *p = nullptr)
        : pos(pos), gCost(g), hCost(h), parent(p) {}

    // Total cost (f = g + h) for priority queue
    int fCost() const { return gCost + hCost; }
};

// Comparator for priority queue (min heap based on fCost)
struct CompareNode
{
    bool operator()(const Node *a, const Node *b) const
    {
        return a->fCost() > b->fCost(); // Lower fCost has higher priority
    }
};

// A* Pathfinding class
class AStar
{
public:
    uint32_t toKey(const Pos &pos) const
    {
        return pos.x | (pos.y << 16);
    }

    /**
     * @brief find a path between Sprite and TargetPos. Sprite must be a temp object
     * since it will be modified.
     *
     * @param boss
     * @param goalPos
     * @return std::vector<Pos>
     */
    std::unique_ptr<std::vector<Pos>> findPath(
        ISprite &boss,
        const Pos goalPos) const
    {
        // const Pos start{.x = boss.x(), .y = boss.y()};
        const Pos startPos = boss.pos();

        // Check if start or goal are valid
        if (!isValid(startPos, boss.granularFactor()) ||
            !isValid(goalPos, boss.granularFactor()))
        {
            const CMap &map = CGame::getMap();
            LOGW("startPos (%d,%d) or goal  (%d/%d) invalid -- expected < %d,%d",
                 startPos.x, startPos.y, goalPos.x, goalPos.y,
                 map.len() * boss.granularFactor(),
                 map.hei() * boss.granularFactor());
            return {};
        }

        // Priority queue for open list
        std::priority_queue<Node *, std::vector<Node *>, CompareNode> openList;
        std::unordered_map<uint32_t, Node *> nodes;
        std::unordered_map<uint32_t, bool> closedList;

        // Create start node
        Node *startNode = new Node(startPos, 0, manhattanDistance(startPos, goalPos));
        openList.push(startNode);
        nodes[toKey(startPos)] = startNode;

        const Pos deltas[] = {
            {-1, 0}, // Up
            {1, 0},  // Down
            {0, -1}, // Left
            {0, 1},  // Right
        };
        // const JoyAim aims[] = {JoyAim::AIM_UP, JoyAim::AIM_DOWN, JoyAim::AIM_LEFT, JoyAim::AIM_RIGHT};

        while (!openList.empty())
        {
            Node *current = openList.top();
            openList.pop();
            closedList[toKey(current->pos)] = true;

            // Reached goal
            if (current->pos.x == goalPos.x && current->pos.y == goalPos.y)
            {
                boss.move(startPos);
                std::unique_ptr<std::vector<Pos>> path = reconstructPath(current);
                cleanup(nodes);
                return path;
            }

            // Explore neighbors
            size_t count = sizeof(deltas) / sizeof(deltas[0]);
            for (size_t i = 0; i < count; ++i)
            {
                boss.move(current->pos.x, current->pos.y);
                const auto &delta = deltas[i];
                const Pos newPos{static_cast<int16_t>(current->pos.x + delta.x), static_cast<int16_t>(current->pos.y + delta.y)};
                const auto newKey = toKey(newPos);
                if (!boss.canMove(g_dirs[i]) || closedList[newKey])
                    continue;

                int newGCost = current->gCost + 1; // Cost to move to neighbor
                Node *neighbor = nodes[newKey];
                if (!neighbor)
                {
                    neighbor = new Node(newPos, newGCost, manhattanDistance(newPos, goalPos), current);
                    nodes[newKey] = neighbor;
                    openList.push(neighbor);
                }
                else if (newGCost < neighbor->gCost)
                {
                    neighbor->gCost = newGCost;
                    neighbor->parent = current;
                    openList.push(neighbor); // Re-push to update priority
                }
            }
        }

        // No path found
        boss.move(startPos);
        cleanup(nodes);
        return {};
    }

private:
    bool isValid(const Pos &pos, const int16_t granularFactor) const
    {
        const CMap &map = CGame::getMap();
        return pos.x >= 0 &&
               pos.y >= 0 &&
               pos.x < map.len() * granularFactor &&
               pos.y < map.hei() * granularFactor;
    }

    // Manhattan distance heuristic
    int manhattanDistance(const Pos &a, const Pos &b) const
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    // Reconstruct path from goal to start
    std::unique_ptr<std::vector<Pos>> reconstructPath(Node *node) const
    {
        std::unique_ptr<std::vector<Pos>> path = std::make_unique<std::vector<Pos>>();
        while (node)
        {
            path->emplace_back(node->pos);
            node = node->parent;
        }
        std::reverse(path->begin(), path->end());
        return path;
    }

    // Clean up dynamically allocated nodes
    void cleanup(std::unordered_map<uint32_t, Node *> &nodes) const
    {
        for (auto &[k, v] : nodes)
            delete v;
    }
};

void CGame::manageBosses(const int ticks)
{
    auto between = [](int a1, int a2, int b1, int b2)
    {
        return a1 < b2 && a2 > b1;
    };

    auto spawnBullet = [](int x, int y, JoyAim aim, bool &valid) -> const CActor
    {
        const uint8_t pu = m_map.at(x, y);
        CActor actor(x, y, TYPE_FIREBALL, aim);
        actor.setPU(pu);
        if (pu == TILES_BLANK)
        {
            m_map.set(x, y, TILES_FIREBALL);
            valid = true;
        }
        else
        {
            valid = false;
        }
        return actor;
    };

    const AStar aStar;
    CActor &player = m_player;
    for (auto &boss : m_bosses)
    {
        const int bx = boss.x() / 2;
        const int by = boss.y() / 2;
        if (boss.speed() != 0 && (ticks % boss.speed() != 0))
            continue;
        if (boss.isPlayerHere() && !isGodMode())
        {
            addHealth(-1);
        }

        if (boss.state() == BossState::Patrol)
        {
            JoyAim aim = static_cast<JoyAim>(rand() & 3);
            if (boss.canMove(aim))
            {
                boss.move(aim);
            }

            if (boss.distance(player) <= CHASE_DISTANCE)
            {
                boss.setState(BossState::Chase);
            }
        }
        else if (boss.state() == BossState::Chase)
        {
            if (boss.distance(player) > CHASE_DISTANCE)
            {
                boss.setState(BossState::Patrol);
                continue;
            }
            if ((rand() & 0xf) == 0 && bx > 2 && by > 0)
            {
                bool valid;
                if (player.y() < by - boss.hitbox().height)
                {
                    const CActor actor = spawnBullet(bx, by - boss.hitbox().height, JoyAim::AIM_UP, valid);
                    if (valid)
                        addMonster(actor);
                }
                else if (player.y() > by + boss.hitbox().height)
                {
                    const CActor actor = spawnBullet(bx, by + boss.hitbox().height, JoyAim::AIM_DOWN, valid);
                    if (valid)
                        addMonster(actor);
                }
                else if (player.x() < bx)
                {
                    const CActor actor = spawnBullet(bx - 1, by - 1, JoyAim::AIM_LEFT, valid);
                    if (valid)
                        addMonster(actor);
                }
                else if (player.x() > bx + boss.hitbox().width)
                {
                    const CActor actor = spawnBullet(bx + boss.hitbox().width, by - 1, JoyAim::AIM_RIGHT, valid);
                    if (valid)
                        addMonster(actor);
                }
            }

            Pos playerPos{static_cast<int16_t>(m_player.x() * CBoss::BOSS_GRANULAR_FACTOR),
                          static_cast<int16_t>(m_player.y() * CBoss::BOSS_GRANULAR_FACTOR)};
            CBoss tmp{boss};
            const auto &path = aStar.findPath(tmp, playerPos);
            if (path->size() > 1)
            {
                const auto &pos = (*path)[1];
                boss.move(pos);
                continue;
            }

            if (bx < player.x() && boss.canMove(JoyAim::AIM_RIGHT))
            {
                boss.move(JoyAim::AIM_RIGHT);
            }

            if (bx > player.x() && boss.canMove(JoyAim::AIM_LEFT))
            {
                boss.move(JoyAim::AIM_LEFT);
            }

            if (by < player.y() && boss.canMove(JoyAim::AIM_DOWN))
            {
                boss.move(JoyAim::AIM_DOWN);
            }

            if (by > player.y() && boss.canMove(JoyAim::AIM_UP))
            {
                boss.move(JoyAim::AIM_UP);
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
        else
        {
            LOGW("unhandled monster type: %.x", actor.type());
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
        else if (defT.type == TYPE_MONSTER)
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
        if (actor.isPlayerThere(aim) && !isGodMode())
        {
            addHealth(def.health);
        }
        m_map.set(actor.x(), actor.y(), actor.getPU());
        deletedMonsters.insert(i);
        m_sfx.emplace_back(sfx_t{.x = actor.x(), .y = actor.y(), .sfxID = SFX_EXPLOSION1, .timeout = SFX_EXPLOSION1_TIMEOUT});
    }
}