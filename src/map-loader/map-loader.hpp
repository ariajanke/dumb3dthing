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

#include "../Defs.hpp"
#include "../PointAndPlaneDriver.hpp"
#include "../Components.hpp"

#include "../platform.hpp"

#include <ariajanke/cul/SubGrid.hpp>

#include <unordered_map>

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

    Slopes(int id_, Real ne_, Real nw_, Real sw_, Real se_):
        AppearanceId(id_),
        nw(nw_), ne(ne_), sw(sw_), se(se_) {}

    bool operator == (const Slopes & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const Slopes & rhs) const noexcept
        { return !are_same(rhs); }

    bool are_same(const Slopes & rhs) const noexcept {
        using Fe = std::equal_to<Real>;
        return    id == rhs.id && Fe{}(nw, rhs.nw) && Fe{}(ne, rhs.ne)
               && Fe{}(sw, rhs.sw) && Fe{}(se, rhs.se);
    }

    Real nw, ne, sw, se;
};

inline Slopes half_pi_rotations(const Slopes & s, int n) {
    if (n < 0) throw std::invalid_argument{""};
    if (n == 0) return s;
    return half_pi_rotations(Slopes{s.id, s.se, s.ne, s.nw, s.sw}, n - 1);
}

inline Slopes translate_y(const Slopes & s, Real y)
    { return Slopes{0, s.ne + y, s.nw + y, s.sw + y, s.se + y}; }

struct Flat final : public AppearanceId {
    Flat() {}
    Flat(int id_, Real y_): AppearanceId(id_), y(y_) {}
    Real y;
};

using Cell = Variant<VoidSpace, Pit, Slopes, Flat>;

using CellSubGrid = ConstSubGrid<Cell>;

class TrianglesAdder final {
public:
    using TriangleVec = std::vector<TriangleSegment>;

    TrianglesAdder(TriangleVec & vec): m_vec(vec) {}

    void add_triangle(TriangleSegment triangle)
        { m_vec.push_back(triangle); }

private:
    TriangleVec & m_vec;
};

class TileGraphicGenerator {
public:
    using LoaderCallbacks = LoaderTask::Callbacks;
    using WallDips = std::array<float, 4>;
    using EntityVec = std::vector<Entity>;

    explicit TileGraphicGenerator(LoaderCallbacks &);

    void setup();

    void create_slope(TrianglesAdder &, Vector2I, const Slopes &);

    void create_flat(TrianglesAdder &, Vector2I, const Flat &, const WallDips &);

    // Can't think of a method fast than O(n^2), though n is always 4 in this
    // case
    // A non real number is returned if there is no such rotation
    static Real rotation_between(const Slopes & rhs, const Slopes & lhs);

    static Slopes sub_minimum_value(const Slopes &);

    static std::array<Vector, 4> get_points_for(const Slopes &);

    static const std::vector<unsigned> & get_common_elements();

private:
    struct SlopesHasher final {
        std::size_t operator () (const Slopes & slopes) const noexcept {
            std::hash<float> hf{};
            return hf(slopes.ne) ^ hf(slopes.nw) ^ hf(slopes.se) ^ hf(slopes.sw) ^ std::hash<int>{}(slopes.id);
        }
    };

    struct SlopesEquality final {
        bool operator () (const Slopes & rhs, const Slopes & lhs) const {
            return cul::is_real(TileGraphicGenerator::rotation_between(rhs, lhs));
        }
    };

    SharedPtr<Texture> & ensure_texture(SharedPtr<Texture> &, const char * filename);

    Tuple<Slopes, SharedPtr<RenderModel>,
          TriangleSegment, TriangleSegment>
        get_slope_model_(const Slopes & slopes, const Vector & translation);

    LoaderCallbacks & m_callbacks;

    SharedPtr<Texture> m_ground_texture, m_tileset4;

    // these things are unable to destruct themselves before the library closes!
    SharedPtr<RenderModel> m_flat_model, m_wall_model;
    std::unordered_map<Slopes, SharedPtr<RenderModel>, SlopesHasher, SlopesEquality> m_slopes_map;
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

using TriangleLinks = std::vector<SharedPtr<TriangleLink>>;

TriangleLinks load_map_graphics
    (TileGraphicGenerator &, CellSubGrid);

Grid<Cell> load_map_cell(const char * layout, const CharToCell &);

template <typename Func>
    Tuple
        <TriangleLinks, Grid<View<TriangleLinks::const_iterator>>>
    add_triangles_and_link_
    (int width, int height, Func && on_add_tile)
{
    using TriangleVec = TrianglesAdder::TriangleVec;
    using std::get;
    Grid<std::pair<std::size_t, std::size_t>> links_grid;
    links_grid.set_size(width, height);
    TriangleVec vec;
    TrianglesAdder adder{vec};
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        links_grid(r).first = vec.size();
        on_add_tile(r, adder);
        links_grid(r).second = vec.size();
    }

    TriangleLinks rv1;
    rv1.reserve(vec.size());
    for (auto & tri : vec)
        { rv1.emplace_back(std::make_shared<TriangleLink>(tri)); }

    Grid<View<TriangleLinks::iterator>> link_grid;
    link_grid.set_size(links_grid.width(), links_grid.height(),
                       View<TriangleLinks::iterator>{ rv1.end(), rv1.end() });
    {
    auto beg = rv1.begin();
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        link_grid(r) = View<TriangleLinks::iterator>{
            beg + links_grid(r).first, beg + links_grid(r).second};
    }
    }

    // now link them together
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
            if (!links_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                this_tri->attempt_attachment_to(other_tri);
        }}
    }}
    Grid<View<TriangleLinks::const_iterator>> rv2;
    rv2.set_size(links_grid.width(), links_grid.height(),
                 View<TriangleLinks::const_iterator>{ rv1.end(), rv1.end() });
    for (Vector2I r; r != rv2.end_position(); r = rv2.next(r)) {
        rv2(r) = View<TriangleLinks::const_iterator>
            { link_grid(r).begin(), link_grid(r).end() };
    }

    return make_tuple(std::move(rv1), std::move(rv2));
}
