/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

*****************************************************************************/

#pragma once

#include "WallTileFactoryBase.hpp"

class TwoWayWallTileFactory final : public WallTileFactoryBase {
    // two known corners
    // two unknown corners
    // bottom:
    // - rectangle whose sides may have different elevations
    // top:
    // - both elevations are fixed
    // wall:
    // - single flat wall
    // if I use a common key, that should simplify things

    bool is_okay_wall_direction(CardinalDirection) const noexcept final;

    KnownCorners make_known_corners() const final;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;
};

class CornerWallTileFactory : public WallTileFactoryBase {
protected:

    bool is_okay_wall_direction(CardinalDirection) const noexcept final;
};

class InWallTileFactory final : public CornerWallTileFactory {
    // three known corners
    // one unknown corner

    KnownCorners make_known_corners() const final;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;
};

class OutWallTileFactory final : public CornerWallTileFactory {
    // one known corner
    // three unknown corners

    KnownCorners make_known_corners() const final;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;
};
