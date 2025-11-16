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
    constexpr JoyAim g_dirs[] = {AIM_UP, AIM_DOWN, AIM_LEFT, AIM_RIGHT};
    constexpr std::array<Pos, JoyAim::TOTAL_AIMS> g_deltas = {
        Pos{-1, 0}, // Up
        Pos{1, 0},  // Down
        Pos{0, -1}, // Left
        Pos{0, 1},  // Right
    };
}

using namespace PathData;

namespace std
{
    template <>
    struct hash<Pos>
    {
        // hashes a key for Pos
        size_t operator()(const Pos &pos) const
        {
            return (static_cast<uint64_t>(pos.x) << 32) | (pos.y & 0xFFFFFFFF);
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
    const int mapLen = map.len() * granularFactor;
    const int mapHei = map.hei() * granularFactor;
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
        for (size_t i = 0; i < g_deltas.size(); ++i)
        {
            const Pos newPos{static_cast<int16_t>(current->pos.x + g_deltas[i].x),
                             static_cast<int16_t>(current->pos.y + g_deltas[i].y)};

            // Check bounds before moving
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei)
                continue;

            if (closedList.find(newPos) != closedList.end())
                continue;

            // save original position
            Pos originalPos{sprite.pos()};

            // Check if move is valid
            sprite.move(newPos);
            if (!sprite.canMove(g_dirs[i]))
            {
                sprite.move(originalPos);
                continue;
            }
            sprite.move(originalPos);

            int newGCost = current->gCost + 1;
            int newHCost = manhattanDistance(newPos, goalPos);

            auto it = nodes.find(newPos);
            if (it == nodes.end() || newGCost < it->second->gCost)
            {
                nodes[newPos] = std::make_unique<Node>(newPos, newGCost, newHCost, current);
                openList.push(nodes[newPos].get());
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
    const int mapLen = map.len() * granularFactor;
    const int mapHei = map.hei() * granularFactor;
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

        for (size_t i = 0; i < g_deltas.size(); ++i)
        {
            Pos newPos = {static_cast<int16_t>(current.x + g_deltas[i].x), static_cast<int16_t>(current.y + g_deltas[i].y)};
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei || visited[newPos])
                continue;

            Pos originalPos = sprite.pos();
            sprite.move(newPos);
            if (sprite.canMove(g_dirs[i]))
            {
                queue.push(newPos);
                visited[newPos] = true;
                parent[newPos] = current;
            }
            sprite.move(originalPos);
        }
    }
    return {};
}

std::vector<JoyAim> LineOfSight::findPath(ISprite &sprite, const Pos &playerPos) const
{
    int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.len() * granularFactor; // Half-tile bounds
    const int mapHei = map.hei() * granularFactor;
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
                sprite.move(next);
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
                const_cast<ISprite &>(sprite).move(next);
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
        {
            //     LOGE("Cannot move from (%d,%d) toward (%d,%d) on line %d",
            //        x, y, goalPos.x, goalPos.y, __LINE__);
            return {};
        }
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
    const int mapLen = map.len() * granularFactor; // Half-tile bounds
    const int mapHei = map.hei() * granularFactor;
    std::vector<Pos> smoothedPath = {path[0]};

    LineOfSight los;
    for (size_t i = 1; i < path.size();)
    {
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
            if (!los.findPath(const_cast<ISprite &>(sprite), end).empty())
            {
                j++;
            }
            else
            {
                break;
            }
        }
        smoothedPath.push_back(path[j - 1]);
        i = j;
    }

    std::vector<JoyAim> directions;
    for (size_t i = 1; i < smoothedPath.size(); ++i)
    {
        const int dx = smoothedPath[i].x - smoothedPath[i - 1].x;
        const int dy = smoothedPath[i].y - smoothedPath[i - 1].y;
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
            LOGE("Invalid smoothed path transition from (%d,%d) to (%d,%d) on line %d",
                 smoothedPath[i - 1].x, smoothedPath[i - 1].y, smoothedPath[i].x, smoothedPath[i].y, __LINE__);
            return {};
        }
    }
    return directions;
}

std::vector<JoyAim> AStarSmooth::findPath(ISprite &sprite, const Pos &playerPos) const
{
    const int granularFactor = sprite.getGranularFactor();
    const CMap &map = CGame::getMap();
    const int mapLen = map.len() * granularFactor; // Half-tile bounds
    const int mapHei = map.hei() * granularFactor;
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

        for (int i = 0; i < 4; ++i)
        {
            const Pos newPos = {static_cast<int16_t>(current->pos.x + g_deltas[i].x), static_cast<int16_t>(current->pos.y + g_deltas[i].y)};
            if (newPos.x < 0 || newPos.x >= mapLen || newPos.y < 0 || newPos.y >= mapHei || closedList[newPos])
                continue;

            const Pos originalPos = sprite.pos();
            const_cast<ISprite &>(sprite).move(newPos);
            if (!sprite.canMove(g_dirs[i]))
            {
                sprite.move(originalPos);
                continue;
            }
            sprite.move(originalPos);

            int newGCost = current->gCost + 1;
            if (!nodes[newPos] || newGCost < nodes[newPos]->gCost)
            {
                nodes[newPos] = std::make_unique<Node>(newPos, newGCost, manhattanDistance(newPos, goalPos), current);
                openList.push(nodes[newPos].get());
            }
        }
    }
    return {};
}

////////////////////////////////////////////////
CPath::Result CPath::followPath(ISprite &sprite, const Pos &playerPos, const IPath &astar)
{
    if (!sprite.isBoss())
        LOGI("sprite: %p[%d,%d] aim:%d p[%d,%d] ptr=%d timeout=%d cache:%lu ttl:%d",
             &sprite, sprite.x(), sprite.y(), sprite.getAim(),
             playerPos.x, playerPos.y,
             m_pathIndex, m_pathTimeout, m_cachedDirections.size(), sprite.getTTL());

    // Check if path is invalid or timed out
    if (m_pathIndex >= m_cachedDirections.size() || m_pathTimeout <= 0)
    {
        m_cachedDirections = astar.findPath(sprite, playerPos);
        m_pathIndex = 0;
        if (!m_pathTimeout)
            m_pathTimeout = PATH_TIMEOUT_MAX;
        if (m_cachedDirections.empty())
        {
            if (!sprite.isBoss())
                LOGI("sprite: %p -- path empty", &sprite);
            return Result::NoValidPath; // No valid path
        }
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
            CGame::getGame()->shadowActorMove(*static_cast<CActor *>(&sprite), aim);
        }
        ++m_pathIndex;
        --m_pathTimeout;
        return Result::MoveSuccesful;
    }

    // Move failed, invalidate cache and recompute next turn
    if (!sprite.isBoss())
        LOGI("sprite: %p -- cannot move", &sprite);
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

    auto checkBound = [](auto a, auto b)
    {
        using T = decltype(a);

        if constexpr (std::is_same_v<T, size_t>)
        {
            if (a > b)
            {
                LOGE("%s: %lu outside expected -- expected < %lu", _S(a), a, b);
                return false;
            }
        }
        else if constexpr (std::is_same_v<T, uint8_t>)
        {
            if (a > b)
            {
                LOGE("%s: %u outside expected -- expected < %u", _S(a), a, b);
                return false;
            }
        }
        else if constexpr (std::is_integral_v<T>)
        {
            if (a > b || a < 0)
            {
                LOGE("%s: %d outside expected -- expected > 0 && < %d", _S(a), a, b);
                return false;
            }
        }
        else
        {
            static_assert(std::is_integral_v<T>, "checkBound only supports integral types");
        }

        return true;
    };

    constexpr size_t DATA_SIZE = 2;
    //////////////////////////////////////
    // Read Path (saved)
    m_cachedDirections.clear();
    size_t pathSize = 0;
    _R(&pathSize, DATA_SIZE);
    checkBound(pathSize, MAX_PATH_SIZE);
    for (size_t i = 0; i < pathSize; ++i)
    {
        JoyAim dir;
        _R(&dir, sizeof(dir));
        m_cachedDirections.emplace_back(dir);
    }
    m_pathIndex = 0;
    _R(&m_pathIndex, DATA_SIZE);
    checkBound(m_pathIndex, MAX_PATH_SIZE);
    m_pathTimeout = 0;
    _R(&m_pathTimeout, DATA_SIZE);
    checkBound(m_pathTimeout, MAX_PATH_SIZE);

    return true;
}

bool CPath::write(IFile &tfile)
{
    auto writefile = [&tfile](auto ptr, auto size)
    {
        return tfile.write(ptr, size) == IFILE_OK;
    };

    constexpr size_t DATA_SIZE = 2;
    const auto pathSize = m_cachedDirections.size();
    _W(&pathSize, DATA_SIZE);
    for (const auto &dir : m_cachedDirections)
    {
        _W(&dir, sizeof(dir));
    }
    _W(&m_pathIndex, DATA_SIZE);
    _W(&m_pathTimeout, DATA_SIZE);

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
    m_pathTimeout = timeout;
}