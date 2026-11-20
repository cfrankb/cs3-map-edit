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
#include <algorithm>
#include <array>
#include <iterator>
#include "map.h"
#include "logger.h"
#include "ai_path.h"
#include "isprite.h"
#include "filemacros.h"
#include "bossdata.h"
#include "shared/IFile.h"

namespace PathData
{
    const AStar aStar;
    const AStarSmooth aStarSmooth;
    const BFS bFS;
    const LineOfSight lineOfSight;

    constexpr int PATH_TIMEOUT_MAX = 10; // Recompute path every 10 turns
    constexpr size_t MAX_PATH_SIZE = 4096;
    // delta+direction pairs kept structurally in sync; must match the
    // CGame::translate()/JoyAim convention used by sprites and by the
    // direction labelling below
    struct Step
    {
        Pos delta;
        JoyAim dir;
    };
    constexpr Step g_steps[] = {
        {Pos{0, -1}, AIM_UP},   // Up
        {Pos{0, 1}, AIM_DOWN},  // Down
        {Pos{-1, 0}, AIM_LEFT}, // Left
        {Pos{1, 0}, AIM_RIGHT}, // Right
    };
}

using namespace PathData;

namespace std
{
    template <>
    struct hash<Pos>
    {
        // packs the int16_t pair (every Pos value) injectively into 32 bits;
        // the previous sign-extended 64-bit shift worked only by accident
        size_t operator()(const Pos &pos) const
        {
            return (static_cast<uint32_t>(static_cast<uint16_t>(pos.x)) << 16) |
                   static_cast<uint16_t>(pos.y);
        }
    };
}

struct Node
{
    Pos pos;
    int gCost, hCost;
    Node *parent;
    Node(const Pos &p, int g, int h, Node *par) : pos(p), gCost(g), hCost(h), parent(par) {}
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

int AStar::manhattanDistance(const Pos &a, const Pos &b) const
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

std::vector<JoyAim> AStar::findPath(ISprite &sprite, const Pos &goalPos) const
{
    const int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.width() * granularFactor;
    const int mapHei = map.height() * granularFactor;
    std::vector<JoyAim> directions;
    const Pos startPos = sprite.pos();

    // Validate start and goal positions
    if (startPos.x < 0 || startPos.x >= mapLen || startPos.y < 0 || startPos.y >= mapHei ||
        goalPos.x < 0 || goalPos.x >= mapLen || goalPos.y < 0 || goalPos.y >= mapHei)
    {
        LOGE("Invalid start (%d,%d) or goal (%d,%d) for map bounds (%d,%d) on line %d",
             startPos.x, startPos.y, goalPos.x, goalPos.y, mapLen, mapHei, __LINE__);
        return {};
    }

    // Priority queue for open list
    std::priority_queue<Node *, std::vector<Node *>, CompareNode> openList;
    std::unordered_map<Pos, std::unique_ptr<Node>> nodes;
    std::unordered_map<Pos, bool> closedList;

    // Create start node
    nodes[startPos] = std::make_unique<Node>(startPos, 0, manhattanDistance(startPos, goalPos), nullptr);
    openList.push(nodes[startPos].get());

    while (!openList.empty())
    {
        Node *current = openList.top();
        openList.pop();

        // BUG-C9: relaxation re-pushes the same node; skip stale copies
        if (closedList.find(current->pos) != closedList.end())
            continue;

        // Reached goal
        if (current->pos == goalPos)
        {
            // Convert path to directions
            Node *node = current;
            std::vector<Pos> path;
            while (node != nullptr)
            {
                path.push_back(node->pos);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());

            // Convert positions to directions
            for (size_t i = 1; i < path.size(); ++i)
            {
                int dx = path[i].x - path[i - 1].x;
                int dy = path[i].y - path[i - 1].y;
                if (dx == 1 && dy == 0)
                    directions.push_back(JoyAim::AIM_RIGHT);
                else if (dx == -1 && dy == 0)
                    directions.push_back(JoyAim::AIM_LEFT);
                else if (dx == 0 && dy == 1)
                    directions.push_back(JoyAim::AIM_DOWN);
                else if (dx == 0 && dy == -1)
                    directions.push_back(JoyAim::AIM_UP);
                else
                {
                    LOGE("Invalid path transition from (%d,%d) to (%d,%d) on line %d",
                         path[i - 1].x, path[i - 1].y, path[i].x, path[i].y, __LINE__);
                    return {};
                }
            }
            return directions;
        }

        closedList[current->pos] = true;

        // Explore neighbors
        for (size_t i = 0; i < std::size(g_steps); ++i)
        {
            const Pos newPos{static_cast<int16_t>(current->pos.x + g_steps[i].delta.x),
                             static_cast<int16_t>(current->pos.y + g_steps[i].delta.y)};

            // Check bounds before moving
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei)
                continue;

            if (closedList.find(newPos) != closedList.end())
                continue;

            // probe canMove from the expanded node so the entered cell is checked
            const Pos originalPos{sprite.pos()};
            sprite.move(current->pos);
            const bool canStep = sprite.canMove(g_steps[i].dir);
            sprite.move(originalPos);
            if (!canStep)
                continue;

            int newGCost = current->gCost + 1;
            int newHCost = manhattanDistance(newPos, goalPos);

            auto it = nodes.find(newPos);
            if (it == nodes.end())
            {
                nodes[newPos] = std::make_unique<Node>(newPos, newGCost, newHCost, current);
                openList.push(nodes[newPos].get());
            }
            else if (newGCost < it->second->gCost)
            {
                // BUG-C9: relax in place. Reassigning the unique_ptr here
                // freed the Node while openList still held its raw pointer.
                // Re-push the same pointer so the heap sees the improved
                // fCost; the stale copy is skipped on pop via closedList.
                it->second->gCost = newGCost;
                it->second->parent = current;
                openList.push(it->second.get());
            }
        }
    }
    // No path found
    return directions; // Empty if no path found
}

std::vector<JoyAim> BFS::findPath(ISprite &sprite, const Pos &goalPos) const
{
    const int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.width() * granularFactor;
    const int mapHei = map.height() * granularFactor;
    const Pos startPos = sprite.pos();

    if (startPos.x < 0 || startPos.x >= mapLen || startPos.y < 0 || startPos.y >= mapHei ||
        goalPos.x < 0 || goalPos.x >= mapLen || goalPos.y < 0 || goalPos.y >= mapHei)
    {
        LOGE("Invalid start (%d,%d) or goal (%d,%d) for map bounds (%d,%d) on line %d",
             startPos.x, startPos.y, goalPos.x, goalPos.y, mapLen, mapHei, __LINE__);
        return {};
    }

    std::queue<Pos> queue;
    std::unordered_map<Pos, Pos> parent;
    std::unordered_map<Pos, bool> visited;
    queue.push(startPos);
    visited[startPos] = true;

    while (!queue.empty())
    {
        Pos current = queue.front();
        queue.pop();

        if (current == goalPos)
        {
            std::vector<JoyAim> directions;
            Pos node = current;
            std::vector<Pos> path;
            while (node != startPos)
            {
                path.push_back(node);
                node = parent[node];
            }
            path.push_back(startPos);
            std::reverse(path.begin(), path.end());

            for (size_t i = 1; i < path.size(); ++i)
            {
                const int dx = path[i].x - path[i - 1].x;
                const int dy = path[i].y - path[i - 1].y;
                if (dx == 1 && dy == 0)
                    directions.push_back(JoyAim::AIM_RIGHT);
                else if (dx == -1 && dy == 0)
                    directions.push_back(JoyAim::AIM_LEFT);
                else if (dx == 0 && dy == 1)
                    directions.push_back(JoyAim::AIM_DOWN);
                else if (dx == 0 && dy == -1)
                    directions.push_back(JoyAim::AIM_UP);
                else
                {
                    LOGE("Invalid path transition from (%d,%d) to (%d,%d) on line %d",
                         path[i - 1].x, path[i - 1].y, path[i].x, path[i].y, __LINE__);
                    return {};
                }
            }
            return directions;
        }

        for (size_t i = 0; i < std::size(g_steps); ++i)
        {
            Pos newPos = {static_cast<int16_t>(current.x + g_steps[i].delta.x), static_cast<int16_t>(current.y + g_steps[i].delta.y)};
            // find(), not operator[]: operator[] would default-insert false
            // keys for every rejected neighbor (only true is ever inserted)
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei ||
                visited.find(newPos) != visited.end())
                continue;

            // probe canMove from the expanded node so the entered cell is checked
            const Pos originalPos = sprite.pos();
            sprite.move(current);
            const bool canStep = sprite.canMove(g_steps[i].dir);
            sprite.move(originalPos);
            if (canStep)
            {
                queue.push(newPos);
                visited[newPos] = true;
                parent[newPos] = current;
            }
        }
    }
    return {};
}

std::vector<JoyAim> LineOfSight::findPath(ISprite &sprite, const Pos &playerPos) const
{
    int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.width() * granularFactor; // Half-tile bounds
    const int mapHei = map.height() * granularFactor;
    const Pos startPos = sprite.pos(); // Half-tile coordinates
    const Pos goalPos = playerPos;

    if (startPos.x < 0 || startPos.x >= mapLen || startPos.y < 0 || startPos.y >= mapHei ||
        goalPos.x < 0 || goalPos.x >= mapLen || goalPos.y < 0 || goalPos.y >= mapHei)
    {
        LOGE("Invalid start (%d,%d) or goal (%d,%d) for map bounds (%d,%d) on line %d",
             startPos.x, startPos.y, goalPos.x, goalPos.y, mapLen, mapHei, __LINE__);
        return {};
    }

    // Generate 4-directional path and check LOS simultaneously
    std::vector<JoyAim> directions;
    int x = startPos.x;
    int y = startPos.y;
    const int dx = goalPos.x - startPos.x;
    const int dy = goalPos.y - startPos.y;
    int stepsX = std::abs(dx);
    int stepsY = std::abs(dy);
    int stepX = dx >= 0 ? 1 : -1;
    int stepY = dy >= 0 ? 1 : -1;
    const Pos originalPos = sprite.pos();

    while (x != goalPos.x || y != goalPos.y)
    {
        bool moved = false;
        // Prioritize x movement if x distance is larger or equal, or y is done
        if (x != goalPos.x && (stepsX >= stepsY || y == goalPos.y))
        {
            Pos next = {static_cast<int16_t>(x + stepX), static_cast<int16_t>(y)};
            if (next.x >= 0 && next.x < mapLen && next.y >= 0 && next.y < mapHei)
            {
                // probe canMove from the current cell so the entered cell is checked
                sprite.move(Pos{static_cast<int16_t>(x), static_cast<int16_t>(y)});
                JoyAim aim = (stepX > 0 ? AIM_RIGHT : AIM_LEFT);
                if (sprite.canMove(aim))
                {
                    directions.push_back(aim);
                    x += stepX;
                    stepsX--;
                    moved = true;
                }
                sprite.move(originalPos);
            }
        }
        // Then try y movement
        if (!moved && y != goalPos.y)
        {
            Pos next = {static_cast<int16_t>(x), static_cast<int16_t>(y + stepY)};
            if (next.x >= 0 && next.x < mapLen && next.y >= 0 && next.y < mapHei)
            {
                // probe canMove from the current cell so the entered cell is checked
                sprite.move(Pos{static_cast<int16_t>(x), static_cast<int16_t>(y)});
                JoyAim aim = (stepY > 0 ? AIM_DOWN : AIM_UP);
                if (sprite.canMove(aim))
                {
                    directions.push_back(aim);
                    y += stepY;
                    stepsY--;
                    moved = true;
                }
                sprite.move(originalPos);
            }
        }
        if (!moved)
            return {};
    }

    return directions;
}

/////////////////////////////////////////////////////////////////////

int AStarSmooth::manhattanDistance(const Pos &a, const Pos &b) const
{
    return abs(a.x - b.x) + abs(a.y - b.y);
}

std::vector<JoyAim> AStarSmooth::smoothPath(const std::vector<Pos> &path, ISprite &sprite) const
{
    if (path.size() < 2)
        return {};
    const int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.width() * granularFactor; // Half-tile bounds
    const int mapHei = map.height() * granularFactor;
    const Pos originalPos = sprite.pos();

    LineOfSight los;
    std::vector<JoyAim> directions;
    for (size_t i = 1; i < path.size();)
    {
        // probe line-of-sight from the actual segment start
        sprite.move(path[i - 1]);
        size_t j = i + 1;
        while (j < path.size())
        {
            Pos start = path[i - 1];
            Pos end = path[j];
            if (start.x < 0 || start.x >= mapLen || start.y < 0 || start.y >= mapHei ||
                end.x < 0 || end.x >= mapLen || end.y < 0 || end.y >= mapHei)
            {
                LOGE("Invalid path coordinates start (%d,%d) or end (%d,%d) for map bounds (%d,%d) on line %d",
                     start.x, start.y, end.x, end.y, mapLen, mapHei, __LINE__);
                break;
            }
            // Reuse LineOfSight to check if direct path is clear in half-tile space
            if (!los.findPath(sprite, end).empty())
            {
                j++;
            }
            else
            {
                break;
            }
        }
        // emit the unit moves from path[i-1] to path[j-1] (LOS is all-or-nothing)
        const auto segment = los.findPath(sprite, path[j - 1]);
        if (segment.empty())
        {
            LOGE("Invalid smoothed path transition from (%d,%d) to (%d,%d)",
                 path[i - 1].x, path[i - 1].y, path[j - 1].x, path[j - 1].y);
            sprite.move(originalPos);
            return {};
        }
        directions.insert(directions.end(), segment.begin(), segment.end());
        i = j;
    }
    sprite.move(originalPos);
    return directions;
}

std::vector<JoyAim> AStarSmooth::findPath(ISprite &sprite, const Pos &playerPos) const
{
    const int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.width() * granularFactor; // Half-tile bounds
    const int mapHei = map.height() * granularFactor;
    const Pos startPos = sprite.pos(); // Half-tile coordinates
    const Pos goalPos = playerPos;

    if (startPos.x < 0 || startPos.x >= mapLen || startPos.y < 0 || startPos.y >= mapHei ||
        goalPos.x < 0 || goalPos.x >= mapLen || goalPos.y < 0 || goalPos.y >= mapHei)
    {
        LOGE("Invalid start (%d,%d) or goal (%d,%d) for map bounds (%d,%d) on line %d",
             startPos.x, startPos.y, goalPos.x, goalPos.y, mapLen, mapHei, __LINE__);
        return {};
    }

    std::priority_queue<Node *, std::vector<Node *>, CompareNode> openList;
    std::unordered_map<Pos, std::unique_ptr<Node>> nodes;
    std::unordered_map<Pos, bool> closedList;

    nodes[startPos] = std::make_unique<Node>(startPos, 0, manhattanDistance(startPos, goalPos), nullptr);
    openList.push(nodes[startPos].get());

    while (!openList.empty())
    {
        Node *current = openList.top();
        openList.pop();

        // BUG-C9: relaxation re-pushes the same node; skip stale copies
        if (closedList.find(current->pos) != closedList.end())
            continue;

        if (current->pos == goalPos)
        {
            std::vector<Pos> path;
            Node *node = current;
            while (node != nullptr)
            {
                path.push_back(node->pos);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
            return smoothPath(path, sprite); // Apply smoothing
        }

        closedList[current->pos] = true;

        for (size_t i = 0; i < std::size(g_steps); ++i)
        {
            const Pos newPos = {static_cast<int16_t>(current->pos.x + g_steps[i].delta.x), static_cast<int16_t>(current->pos.y + g_steps[i].delta.y)};
            // find(), not operator[]: BUG-C9's pop skip treats map membership
            // as closed, so no false-valued probes may enter closedList
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei ||
                closedList.find(newPos) != closedList.end())
                continue;

            // probe canMove from the expanded node so the entered cell is checked
            const Pos originalPos = sprite.pos();
            sprite.move(current->pos);
            const bool canStep = sprite.canMove(g_steps[i].dir);
            sprite.move(originalPos);
            if (!canStep)
                continue;

            int newGCost = current->gCost + 1;
            // find() first: operator[] here default-inserted a *null* entry for
            // every probed neighbor and re-hashed on each access
            auto it = nodes.find(newPos);
            if (it == nodes.end())
            {
                nodes[newPos] = std::make_unique<Node>(newPos, newGCost, manhattanDistance(newPos, goalPos), current);
                openList.push(nodes[newPos].get());
            }
            else if (newGCost < it->second->gCost)
            {
                // BUG-C9: relax in place (see AStar); replacing the unique_ptr
                // would free the Node still referenced by openList
                it->second->gCost = newGCost;
                it->second->parent = current;
                openList.push(it->second.get());
            }
        }
    }
    return {};
}

////////////////////////////////////////////////
CPath::Result CPath::followPath(ISprite &sprite, const Pos &playerPos, const IPath &astar)
{
    // Maintainer ruling 2026-11-18: standing on the goal needs no search;
    // skip the (expensive, always-empty) recompute instead of re-running it.
    if (sprite.pos() == playerPos)
        return Result::NoValidPath;

    // Check if path is invalid or timed out
    if (m_pathIndex >= m_cachedDirections.size() || m_pathTimeout == 0)
    {
        m_cachedDirections = astar.findPath(sprite, playerPos);
        m_pathIndex = 0;
        if (!m_pathTimeout)
            m_pathTimeout = PATH_TIMEOUT_MAX;
        if (m_cachedDirections.empty())
            return Result::NoValidPath; // No valid path
    }

    // Try the next direction
    const JoyAim aim = m_cachedDirections[m_pathIndex];
    sprite.setAim(aim);
    if (sprite.canMove(aim))
    {
        if (sprite.isBoss())
        {
            sprite.move(aim);
        }
        else
        {
            // unchecked downcast was UB for any other ISprite; fail safe
            CActor *actor = dynamic_cast<CActor *>(&sprite);
            if (!actor)
            {
                LOGE("CPath::followPath: non-boss sprite %p is not a CActor",
                     static_cast<void *>(&sprite));
                return Result::Blocked;
            }
            // A pathing CActor must never occupy the player's cell: the
            // CActor::move tile swap would displace the player's stamped
            // map identity. MONSTERV3 canMove() admits that cell only so
            // the A* goal is reachable.
            if (actor->isPlayerThere(aim))
                return Result::NoValidPath;
            CGame::getGame()->shadowActorMove(*actor, aim);
        }
        ++m_pathIndex;
        --m_pathTimeout;
        return Result::MoveSuccesful;
    }

    // Move failed, invalidate cache and recompute next turn
    m_cachedDirections.clear();
    m_pathIndex = 0;
    m_pathTimeout = 0;
    return Result::Blocked;
}

bool CPath::read(IFile &sfile)
{
    auto readfile = [&sfile](auto ptr, auto size) -> bool
    {
        return sfile.read(ptr, size) == IFILE_OK;
    };

    // all three gated fields are size_t after the 0x0011 uint16-local reads
    auto checkBound = [](const char *name, size_t value, size_t max) -> bool
    {
        if (value > max)
        {
            LOGE("%s: %zu outside expected -- expected <= %zu", name, value, max);
            return false;
        }
        return true;
    };

    constexpr size_t DATA_SIZE = 2;
    //////////////////////////////////////
    // Read Path (saved)
    m_cachedDirections.clear();
    uint16_t u16 = 0;
    _R(&u16, DATA_SIZE);
    size_t pathSize = u16;
    if (!checkBound("pathSize", pathSize, MAX_PATH_SIZE))
        return false;
    for (size_t i = 0; i < pathSize; ++i)
    {
        JoyAim dir;
        _R(&dir, sizeof(dir));
        // cached directions only ever hold AIM_UP..AIM_RIGHT (see g_steps);
        // AIM_NONE/0xff and other garbage never legitimately reach the wire
        if (dir >= TOTAL_AIMS)
        {
            LOGE("CPath::read: corrupt direction %.2x", dir);
            return false;
        }
        m_cachedDirections.emplace_back(dir);
    }
    m_pathIndex = 0;
    _R(&u16, DATA_SIZE);
    m_pathIndex = u16;
    if (!checkBound("m_pathIndex", m_pathIndex, MAX_PATH_SIZE))
        return false;
    m_pathTimeout = 0;
    _R(&u16, DATA_SIZE);
    m_pathTimeout = u16;
    if (!checkBound("m_pathTimeout", m_pathTimeout, MAX_PATH_SIZE))
        return false;

    return true;
}

bool CPath::write(IFile &tfile)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    constexpr size_t DATA_SIZE = 2;
    // values are bounded by MAX_PATH_SIZE / PATH_TIMEOUT_MAX, so the
    // uint16 wire fields cannot truncate
    const uint16_t pathSize = static_cast<uint16_t>(m_cachedDirections.size());
    _W(&pathSize, DATA_SIZE);
    for (const auto &dir : m_cachedDirections)
    {
        _W(&dir, sizeof(dir));
    }
    const uint16_t pathIndex = static_cast<uint16_t>(m_pathIndex);
    const uint16_t pathTimeout = static_cast<uint16_t>(m_pathTimeout);
    _W(&pathIndex, DATA_SIZE);
    _W(&pathTimeout, DATA_SIZE);

    return true;
}

CPath::CPath()
{
    m_pathIndex = 0;
    m_pathTimeout = 0;
}

const IPath *CPath::getPathAlgo(const uint8_t algo)
{
    if (algo == BossData::ASTAR)
    {
        return &PathData::aStar;
    }
    else if (algo == BossData::LOS)
    {
        return &PathData::lineOfSight;
    }
    else if (algo == BossData::BFS)
    {
        return &PathData::bFS;
    }
    else if (algo == BossData::ASTAR_SMOOTH)
    {
        return &PathData::aStarSmooth;
    }
    else
    {
        LOGE("unsupported ai algo: %u", algo);
        return nullptr;
    }
}

void CPath::setTimeout(int timeout)
{
    // m_pathTimeout is size_t: a negative argument would wrap to a huge
    // value and disable path expiry
    m_pathTimeout = timeout < 0 ? 0 : static_cast<size_t>(timeout);
}