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

    bool is_okay_wall_direction(CardinalDirection dir) const noexcept final {
        using Cd = CardinalDirection;
        constexpr static std::array k_list = { Cd::n, Cd::e, Cd::s, Cd::w };
        return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
    }

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;

    KnownCorners make_known_corners() const final {
        using Cd = CardinalDirection;
        using Corners = KnownCorners;
        switch (direction()) {
        case Cd::n:
            return Corners{}.nw(false).sw(true ).se(true ).ne(false);
        case Cd::s:
            return Corners{}.nw(true ).sw(false).se(false).ne(true );
        case Cd::e:
            return Corners{}.nw(true ).sw(true ).se(false).ne(false);
        case Cd::w:
            return Corners{}.nw(false).sw(false).se(true ).ne(true );
        default: throw BadBranchException{__LINE__, __FILE__};
        }
    }
};

// in wall
// top: flat is always true flat
// bottom: not so, variable elevations
// wall "elements": can be shared between instances
// - whole squares
// - triangle peices
//
// both in and out walls split at the same place
//
// ensure top
// ensure bottom
// ensure wall(s)
// in and out have two
// two-way has only one

class CornerWallTileFactory : public WallTileFactoryBase {
protected:
    bool is_okay_wall_direction(CardinalDirection dir) const noexcept final {
        using Cd = CardinalDirection;
        constexpr static std::array k_list = { Cd::ne, Cd::nw, Cd::se, Cd::sw };
        return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
    }
};

class InWallTileFactory final : public CornerWallTileFactory {

    // three known corners
    // one unknown corner

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;

    KnownCorners make_known_corners() const final;
};

class OutWallTileFactory final : public CornerWallTileFactory {
    // one known corner
    // three unknown corners

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const final;

    KnownCorners make_known_corners() const final {
        using Cd = CardinalDirection;
        using Corners = KnownCorners;
        switch (direction()) {
        case Cd::nw:
            return Corners{}.nw(false).sw(false).se(true ).ne(false);
        case Cd::sw:
            return Corners{}.nw(false).sw(false).se(false).ne(true );
        case Cd::se:
            return Corners{}.nw(true ).sw(false).se(false).ne(false);
        case Cd::ne:
            return Corners{}.nw(false).sw(true ).se(false).ne(false);
        default: throw std::invalid_argument{"Bad direction"};
        }
    }
};

inline InWallTileFactory::KnownCorners InWallTileFactory::make_known_corners() const {
    using Cd = CardinalDirection;
    using Corners = KnownCorners;
    switch (direction()) {
    case Cd::nw:
        return Corners{}.nw(false).sw(true ).se(true ).ne(true );
    case Cd::sw:
        return Corners{}.nw(true ).sw(false).se(true ).ne(true );
    case Cd::se:
        return Corners{}.nw(true ).sw(true ).se(false).ne(true );
    case Cd::ne:
        return Corners{}.nw(true ).sw(true ).se(true ).ne(false);
    default: throw std::invalid_argument{"Bad direction"};
    }
}
