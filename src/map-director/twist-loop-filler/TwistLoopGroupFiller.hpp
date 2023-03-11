/******************************************************************************

    GPLv3 License
    Copyright (c) 2023 Aria Janke

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

#include "../ProducableTileFiller.hpp"
#include "../../RenderModel.hpp"
#include "../ViewGrid.hpp"

#include <ariajanke/cul/Util.hpp>

#include <optional>

template <typename T>
using Optional = std::optional<T>;

// how does tile generation work here?
// only load in geometry relevant to a tile? (yes)

class TileSetXmlGrid;
class TwistTileGroup {
public:
    using Rectangle = cul::Rectangle<Real>;

    virtual ~TwistTileGroup() {}

    virtual void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const = 0;

    Vector2I group_start() const { return m_group_start; }

    // stay in integer land for as long as possible
    void load(const RectangleI &, const TileSetXmlGrid &, Platform &);

protected:
    virtual void load_(const RectangleI &, const TileSetXmlGrid &, Platform &) = 0;

private:
    Vector2I m_group_start;
};

inline void TwistTileGroup::load
    (const RectangleI & rect, const TileSetXmlGrid & xml_grid,
     Platform & platform)
{
    m_group_start = top_left_of(rect);
    load_(rect, xml_grid, platform);
}

// north-south
// left/right twisting

enum class TwistDirection {
    left, right
};

enum class TwistPathDirection {
    north_south,
    east_west
};

// could grab triangles for
// ... texturing?
//     use a function

struct TexturingAdapter {
    virtual ~TexturingAdapter() {}

    virtual Vector2 operator() (const Vector2 & position_on_twisty) const = 0;  
};

class CapTexturingAdapter final : public TexturingAdapter {
public:
    CapTexturingAdapter(const Vector2 & offset, const Size2 & twisty_size):
        m_offset(offset),
        m_twisty_size(twisty_size) {}

    Vector2 operator() (const Vector2 & position_on_twisty) const final {
        auto x = position_on_twisty.x / m_twisty_size.width;
        return m_offset
            + Vector2{x, relative_texture_position_y(position_on_twisty)};
    }

private:
    Real relative_texture_position_y(const Vector2 & position_on_twisty) const {
        auto cap_thershold = m_twisty_size.height - 1;
        if (position_on_twisty.y <= 1) {
            return position_on_twisty.y;
        }
        if (position_on_twisty.y >= cap_thershold) {
            return position_on_twisty.y - cap_thershold;
        }
        return std::fmod(position_on_twisty.y, Real(1.));
    }

    Vector2 m_offset;
    Size2 m_twisty_size;
};


using VertexTriangle = std::array<Vertex, 3>;

// how do I split these among tiles?
// have segment running along path direction (tile length)
// (I could just put them on one tile per segment...)
// why? region boundries, and not cutting off in the middle
// starts off at all tiles, shrinks to middle two (1/4)
// all tiles at 1/2, middle two 3/4, all again at end

ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2I & twisty_size, TwistDirection, TwistPathDirection,
     const TexturingAdapter &, Real breaks_per_tile);

class TwistyTileTValueRange final {
public:
    TwistyTileTValueRange
        (const Size2I & twisty_size, const Vector2I & tile_pos):
        m_low_t (Real(tile_pos.x    ) / twisty_size.width),
        m_high_t(Real(tile_pos.x + 1) / twisty_size.width)
    {}

    Real low_t() const { return m_low_t; }

    Real high_t() const { return m_high_t; }

    bool contains(Real t) const { return t >= low_t() && t <= high_t(); }

private:
    Real m_low_t;
    Real m_high_t;
};

// I am a computation class, an instance is essentially the return value
/// computes boundries of tile geometry in "t" values
class TwistyTileTValueLimits final {
public:

    static Optional<TwistyTileTValueLimits> find
        (const Size2I & twisty_size, const Vector2I & tile_pos);

    static Optional<Real> intersecting_t_value
        (const Size2I & twisty_size, int strip_x,
         const TwistyTileTValueRange &);

    Real low_t_limit() const { return m_low_t_limit; }

    Real high_t_limit() const { return m_high_t_limit; }

private:
    TwistyTileTValueLimits(Real low_t_limit_, Real high_t_limit_):
        m_low_t_limit(low_t_limit_), m_high_t_limit(high_t_limit_) {}

    // the only real "state", are the answers for a "computation" class
    // should this be a struct then? idk
    Real m_low_t_limit;
    Real m_high_t_limit;
};

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, T>
    min_magnitude(const T & lhs, const T & rhs)
{ return magnitude(lhs) < magnitude(rhs) ? lhs : rhs; }

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, T>
    max_magnitude(const T & lhs, const T & rhs)
{ return magnitude(lhs) >= magnitude(rhs) ? lhs : rhs; }

// offsets should not be confused with distance
class TwistyStripSpineOffsets final {
public:
    static constexpr const Vector k_twisty_origin{-0.5, 0, -0.5};

    // tile_pos_x is being used to determine which side of the spine we're on
    static Optional<TwistyStripSpineOffsets> find
        (int twisty_width, int strip_pos_x, Real t_value)
    {
        if (t_value < 0 || t_value > 1) return {};
        auto max_x = (Real(twisty_width) / 2)*std::cos(t_value*2*k_pi);
        // if our strip is beyond the maximum x, then there are no radii
        auto [spine_x, edge_x] = spine_and_edge_x_offsets(twisty_width, strip_pos_x);
        if (magnitude(spine_x) > magnitude(max_x)) return {};
        // turn an x into a radius...
        return TwistyStripSpineOffsets{spine_x, min_magnitude(edge_x, max_x)};
    }

    // there maybe positive or negative
    Real spine() const { return m_spine; }

    Real edge() const { return m_edge; }

    // just the strip, unconstrained by twisty
    static Tuple<Real, Real> spine_and_edge_x_offsets
        (int twisty_width, int strip_pos_x)
    {
        if (   twisty_width / 2 == strip_pos_x
            && twisty_width % 2 == 1)
        { return make_tuple(0, 0.5); }
        auto low_side  = strip_pos_x;
        auto high_side = strip_pos_x + 1;
        auto spine_pos = Real(twisty_width) / 2;
        auto strip_low  = low_side  - spine_pos;
        auto strip_high = high_side - spine_pos;
        return make_tuple(min_magnitude(strip_low, strip_high),
                          max_magnitude(strip_low, strip_high));
    }

    template <typename ... Types>
    static Real edge_x_offset(Types &&... args)
        { return std::get<1>(spine_and_edge_x_offsets(std::forward<Types>(args)...)); }

private:
    TwistyStripSpineOffsets(Real spine_, Real edge_):
        m_spine(spine_), m_edge(edge_) {}

    Real m_spine, m_edge;
};

// t value determines direction
// these radii limits determine how far we go to reach these points on the tile
class TwistyStripRadii final {
public:
    static constexpr const Vector k_twisty_origin{-0.5, 0, -0.5};

    // tile_pos_x is being used to determine which side of the spine we're on
    static Optional<TwistyStripRadii> find
        (int twisty_width, int strip_pos_x, Real t_value)
    {
        using Offsets = TwistyStripSpineOffsets;
        return find
            (Offsets::find(twisty_width, strip_pos_x, t_value), t_value);
    }

    static Optional<TwistyStripRadii> find
        (const Optional<TwistyStripSpineOffsets> & offsets, Real t_value)
    {
        if (!offsets) return {};
        // I need the y component
        auto cos_t = std::cos(t_value*2*k_pi);
        return TwistyStripRadii{cos_t*offsets->spine(), cos_t*offsets->edge()};
    }

    // these should only be positive reals
    Real spine() const { return m_spine; }

    Real edge() const { return m_edge; }

private:
    TwistyStripRadii(Real spine_, Real edge_):
        m_spine(verify_positive_real("TwistyStripRadii", spine_)),
        m_edge (verify_positive_real("TwistyStripRadii", edge_ )) {}

    static Real verify_positive_real(const char * caller, Real x) {
        if (x > 0) return x;
        throw std::invalid_argument
            {  "TwistyStripRadii::" + std::string{caller}
             + ": expect a positive real number"         };
    }

    Real m_spine, m_edge;
};

Vector to_twisty_offset(const Real & radius, Real t);

Vector to_twisty_spine(const Size2 & twisty_size, Real t);

class TwistyTileEdgePoints final {
public:
    class PointPair final {
    public:
        PointPair() {}

        PointPair(const Vector2 & on_tile_, const Vector & in_3d_):
            m_on_tile(on_tile_), m_in_3d(in_3d_) {}

        Vector2 on_tile() const { return m_on_tile; }

        Vector in_3d() const { return m_in_3d; }

    private:
        Vector2 m_on_tile;
        Vector m_in_3d;
    };

    TwistyTileEdgePoints
        (const Size2I & twisty_size, int strip_pos_x, Real t_value)
    {
        auto offsets = TwistyStripSpineOffsets::find
            (twisty_size.width, strip_pos_x, t_value);
        auto radii   = TwistyStripRadii ::find(offsets, t_value);
        if (!offsets) {
            m_count = 0;
            return;
        }
        if (!radii)
            { throw BadBranchException{__LINE__, __FILE__}; }
        // v is this really normalized?
        auto dir = normalize( to_twisty_offset(1, t_value) );
        // but now I need to reverse it :/
        auto y_pos = Real(twisty_size.height)*t_value;
        auto spine_pos = Real(twisty_size.width) / 2;
        m_elements[0] = PointPair
            {Vector2{offsets->edge() + spine_pos, y_pos}, dir*radii->edge()};
        if (are_very_close(radii->edge(), radii->spine())) {
            m_count = 1;
        } else {
            m_elements[1] = PointPair
                {Vector2{offsets->spine() + spine_pos, y_pos},
                 dir*radii->spine()};
            m_count = 2;
        }
    }

    int count() const { return m_count; }

    auto begin() const { return m_elements.begin(); }

    auto end() const { return begin() + count(); }

private:
    int m_count = 0;
    std::array<PointPair, 2> m_elements;
};

class TwistyTilePoints final {
public:
    using PointPair = TwistyTileEdgePoints::PointPair;

    TwistyTilePoints
        (const TwistyTileEdgePoints & lhs_points,
         const TwistyTileEdgePoints & rhs_points)
    {
        for (auto & points : { lhs_points, rhs_points }) {
            for (auto & point_pair : points) {
                m_elements[m_count++] = point_pair;
            }
        }
    }

    PointPair operator [] (int i) const {
        if (i < m_count)
            return m_elements[i];
        throw std::out_of_range{""};
    }

    int count() const { return m_count; }

private:
    int m_count = 0;
    std::array<PointPair, 4> m_elements;
};


// an entire chain of methods, how can this be tested?
// one depending on another

// some t breaks are unavoidable depending on limits on tiles, so that we can
// assure all triangles get linked
// (number of breaks - 1)*2 represents the number of triangles that will be
// generated
std::vector<Real> find_unavoidable_t_breaks_for_twisty
    (const Size2I & twisty_size);

std::vector<Real> verify_ordered(const char * caller, std::vector<Real> && reals);
std::vector<Real> verify_at_least_two(const char * caller, std::vector<Real> && reals);
std::vector<Real> verify_all_within_zero_to_one(const char * caller, std::vector<Real> && reals);

std::vector<Real> pad_t_breaks_until_target
    (int target_number_of_breaks, std::vector<Real> &&);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> &,
     const Size2I & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end);
#if 0
ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2I & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment);

ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2 & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment);
#endif

#if 0
void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const Size2I & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end);
#endif
#if 0 // temp, should be revived
void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTilePointLimits & low_lims,
     const TwistyTilePointLimits & high_lims);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTilePointLimits & low_lims,
     const TwistyTilePointLimits & high_lims);
#endif

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTileEdgePoints & low_points,
     const TwistyTileEdgePoints & high_points);

class NorthSouthTwistTileGroup final : public TwistTileGroup {
public:
    static constexpr const Real k_breaks_per_segment = 2;

    void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const final;

private:
    void load_
        (const RectangleI &, const TileSetXmlGrid &, Platform &) final;

    Grid<SharedPtr<const RenderModel>> m_group_models;
    ViewGrid<TriangleSegment> m_collision_triangles;
};

class TwistLoopTile final : public ProducableTile {
public:
    TwistLoopTile
        (const Vector2I & position_in_map,
         const Vector2I position_in_group,
         const SharedPtr<TwistTileGroup> & tile_group):
        m_position_in_map(position_in_map),
        m_position_in_group(position_in_group),
        m_twist_group(tile_group) {}

    void operator () (const Vector2I & maps_offset,
                      EntityAndTrianglesAdder & adder, Platform & platform) const final
        { (*m_twist_group)(m_position_in_group, m_position_in_map + maps_offset, adder, platform); }

private:
    Vector2I m_position_in_map;
    Vector2I m_position_in_group;
    SharedPtr<TwistTileGroup> m_twist_group;
};

class TwistLoopGroupFiller final : public ProducableTileFiller {
public:
    void load(const TileSetXmlGrid & xml_grid, Platform & platform);

    UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> &, UnfinishedTileGroupGrid &&) const final;

private:
    Grid<SharedPtr<TwistTileGroup>> m_tile_groups;
};
