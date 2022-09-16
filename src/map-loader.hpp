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
#include "Components.hpp"

#include "platform/platform.hpp"

#include <common/SubGrid.hpp>

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


class TileGraphicGenerator {
public:
    using LoaderCallbacks = LoaderTask::Callbacks;
    using WallDips = std::array<float, 4>;
    using TriangleVec = std::vector<SharedPtr<TriangleSegment>>;
    using EntityVec = std::vector<Entity>;

    TileGraphicGenerator(TriangleVec &, LoaderCallbacks &);
#   if 0
    TileGraphicGenerator(EntityVec &, TriangleVec &, Platform::ForLoaders &);
#   endif
    void setup();

    void create_slope(Vector2I, const Slopes &);

    void create_flat(Vector2I, const Flat &, const WallDips &);

    // Can't think of a method fast than O(n^2), though n is always 4 in this
    // case
    // A non real number is returned if there is no such rotation
    static Real rotation_between(const Slopes & rhs, const Slopes & lhs);

    static Slopes sub_minimum_value(const Slopes &);

    std::size_t triangle_count() const noexcept
        { return m_triangles_out.size(); }

    cul::View<TriangleVec::iterator> triangles_view()
        { return cul::View{ m_triangles_out.begin(), m_triangles_out.end() }; }

    TriangleVec & full_triangles_vec() { return m_triangles_out; }
#   if 0
    std::vector<Entity> give_entities()
        { return std::move(m_entities_out); }
#   endif

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
          SharedPtr<TriangleSegment>, SharedPtr<TriangleSegment>>
        get_slope_model_(const Slopes & slopes, const Vector & translation);


    TriangleVec & m_triangles_out;
#   if 0
    EntityVec & m_entities_out;
    Platform::ForLoaders & m_platform;
#   endif
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

Tuple<std::vector<TriangleLinks>/*,
      std::vector<Entity>       */> load_map_graphics
    (TileGraphicGenerator &, CellSubGrid);


Grid<Cell> load_map_cell(const char * layout, const CharToCell &);

class TrianglesAdder final {
public:
    using TriangleVec = std::vector<SharedPtr<TriangleSegment>>;

    TrianglesAdder(TriangleVec & vec): m_vec(vec) {}

    void add_triangle(SharedPtr<TriangleSegment> ptr)
        { m_vec.push_back(ptr); }

private:
    TriangleVec & m_vec;
};

using TrianglePtrsViewGrid =
    Grid<cul::View<std::vector<SharedPtr<TriangleSegment>>::const_iterator>>;

template <typename Func>
Tuple<std::vector<TriangleLinks>, TrianglePtrsViewGrid>
    add_triangles_and_link
    (int width, int height, Func && on_add_tile,
     TrianglesAdder::TriangleVec * outvec = nullptr)
{
    using TriangleVec = TrianglesAdder::TriangleVec;
    using std::get;
    Grid<std::pair<std::size_t, std::size_t>> links_grid;
    links_grid.set_size(width, height);
    TriangleVec infn_vec;
    TriangleVec & vec = outvec ? *outvec : infn_vec;
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        links_grid(r).first = vec.size();
        on_add_tile(r, TrianglesAdder{vec});
        links_grid(r).second = vec.size();
    }

    using TrisItr = TileGraphicGenerator::TriangleVec::const_iterator;
    using TrisView = cul::View<TrisItr>;
    TrianglePtrsViewGrid triangles_grid;
    triangles_grid.set_size(links_grid.width(), links_grid.height(),
                            TrisView{vec.end(), vec.end()});
    {
    auto beg = vec.begin();
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        triangles_grid(r) = TrisView{beg + links_grid(r).first, beg + links_grid(r).second};
    }
    }
    using Links = TriangleLinks;
    std::vector<Links> links;
    // now link them together
    for (Vector2I r; r != triangles_grid.end_position(); r = triangles_grid.next(r)) {
    for (auto this_tri : triangles_grid(r)) {
        if (!this_tri) continue;
        links.emplace_back(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
        if (!links_grid.has_position(v)) continue;
        for (auto other_tri : triangles_grid(v)) {
            if (!other_tri) continue;
            if (this_tri == other_tri) continue;
            auto & link = links.back();
            link.attempt_attachment_to(other_tri);
        }}
    }}
    return make_tuple(links, triangles_grid);
}
