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

#include "Defs.hpp"
#include "PointAndPlaneDriver.hpp"

#include "platform/platform.hpp"

#include <common/SubGrid.hpp>

#include <variant>

// Entities seem like a good solution for rendering, it solves the "forest of
// trees" problem very easily

using cul::ConstSubGrid;

struct AppearanceId {
    int id = 0;
protected:
    AppearanceId() {}
    AppearanceId(int id_): id(id_) {}
};

struct VoidSpace final {};

struct Pit final {};

struct EndOfRow final {};

struct Slopes final : public AppearanceId {
    Slopes() {}

    Slopes(int id_, float ne_, float nw_, float sw_, float se_):
        AppearanceId(id_),
        nw(nw_), ne(ne_), sw(sw_), se(se_) {}

    bool operator == (const Slopes & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const Slopes & rhs) const noexcept
        { return !are_same(rhs); }

    bool are_same(const Slopes & rhs) const noexcept {
        using Fe = std::equal_to<float>;
        return    id == rhs.id && Fe{}(nw, rhs.nw) && Fe{}(ne, rhs.ne)
               && Fe{}(sw, rhs.sw) && Fe{}(se, rhs.se);
    }

    float nw, ne, sw, se;
};

struct Flat final : public AppearanceId {
    Flat() {}
    Flat(int id_, float y_): AppearanceId(id_), y(y_) {}
    float y;
};

using Cell = Variant<VoidSpace, Pit, Slopes, Flat>;

using CellSubGrid = ConstSubGrid<Cell>;


// In terms of maintenance?
// I can imagine many things not even returning triangles for physics, some
// cases where there are many per tile
//
// But always a grid form... so I can least be sure about neighboring/limiting
// attachment search(?)

class TileGraphicGenerator {
public:
    using WallDips = std::array<float, 4>;
    using TriangleVec = std::vector<SharedPtr<TriangleSegment>>;
    using EntityVec = std::vector<Entity>;
    // ...
    virtual ~TileGraphicGenerator() {}

    virtual void create_slope
        (EntityVec &, TriangleVec &, Vector2I, const Slopes &) = 0;
    // each flat is accompanied by each wall dips down from that flat
    // so with that in mind, I need to know which sides dip down (if any)
    virtual void create_flat
        (EntityVec &, TriangleVec &, Vector2I, const Flat &, const WallDips &) = 0;

    static UniquePtr<TileGraphicGenerator> make_builtin(Platform::ForLoaders &);

    static UniquePtr<TileGraphicGenerator> make_no_graphics();

    // Can't think of a method fast than O(n^2), though n is always 4 in this
    // case
    // A non real number is returned if there is no such rotation
    static Real rotation_between(const Slopes & rhs, const Slopes & lhs);

    static Slopes sub_minimum_value(const Slopes &);
};

class CharToCell {
public:
    using MaybeCell = Variant<VoidSpace, Pit, Slopes, Flat, EndOfRow>;

    virtual ~CharToCell() {}

    MaybeCell operator () (char c) const;

    static Cell to_cell(const MaybeCell &);

    static MaybeCell to_maybe_cell(const Cell &);

    static const CharToCell & default_instance();

protected:
    virtual Cell to_cell(char) const = 0;
};

Tuple<std::vector<TriangleLinks>,
      std::vector<Entity>       > load_map_graphics
    (TileGraphicGenerator &, CellSubGrid);

Grid<Cell> load_map_cell(const char * layout, const CharToCell &);

void run_map_loader_tests();
