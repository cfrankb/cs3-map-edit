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

#pragma once
#include <vector>
#include "sprtypes.h"
#include "rect.h"
#include "joyaim.h"
#include "map.h"

class ISprite;

class IPath
{
public:
    virtual std::vector<JoyAim> findPath(ISprite &sprite, const Pos &playerPos) const = 0;
};

// A* Pathfinding class
class AStar : public IPath
{
public:
    std::vector<JoyAim> findPath(ISprite &sprite, const Pos &playerPos) const override;

private:
    int manhattanDistance(const Pos &a, const Pos &b) const;
};

// A* Pathfinding class
class AStarSmooth : public IPath
{
public:
    std::vector<JoyAim> findPath(ISprite &sprite, const Pos &playerPos) const override;

private:
    int manhattanDistance(const Pos &a, const Pos &b) const;
    std::vector<JoyAim> smoothPath(const std::vector<Pos> &path, ISprite &sprite) const;
};

// BFS Pathfinding class
class BFS : public IPath
{
public:
    std::vector<JoyAim> findPath(ISprite &sprite, const Pos &playerPos) const override;
};

// Line-of-Sight Pathfinding class
class LineOfSight : public IPath
{
public:
    std::vector<JoyAim> findPath(ISprite &sprite, const Pos &playerPos) const override;
};
