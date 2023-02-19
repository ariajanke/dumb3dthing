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

// how does tile generation work here?
// only load in geometry relevant to a tile? (yes)

class TileSetXmlGrid;
class TwistTileGroup {
public:
    using Rectangle = cul::Rectangle<Real>;

    virtual ~TwistTileGroup() {}

    virtual void load(const Rectangle &, const TileSetXmlGrid &, Platform &) = 0;

    virtual void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const = 0;
};

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
    (const Size2 & twisty_size, TwistDirection, TwistPathDirection,
     const TexturingAdapter &, Real breaks_per_tile);

// I am a computation class, an instance is essentially the return value
/// computes boundries of tile geometry in "t" values
class TwistyTileTValueLimits final {
public:
    TwistyTileTValueLimits
        (const Size2 & twisty_size, const Vector2I & tile_pos);

    Real low_t_limit() const { return m_low_t_limit; }

    Real high_t_limit() const { return m_high_t_limit; }

private:
    // the only real "state", are the answers for a "computation" class
    // should this be a struct then? idk
    Real m_low_t_limit;
    Real m_high_t_limit;
};

// feels like this is missing a direction (from the spine)
class TwistyTileRadiiLimits final {
public:
    static constexpr const Vector k_twisty_origin{-0.5, 0, -0.5};

    TwistyTileRadiiLimits
        (const Size2 & twisty_size, const Vector2I & tile_pos, Real t_value);

    Real spine_radius() const { return m_spine; }

    Real edge_radius() const { return m_edge; }

    static Tuple<Real, Real, Real> low_high_x_edges_and_low_dir
        (const Size2 & twisty_size, Real t_value);

    static Tuple<Real, Real> spine_and_edge_side_x
        (Real twisty_width, Real tile_pos_x);

private:
    Real m_t_value, m_spine, m_edge;

};

/// points which are both limited by tile and radial boundries
class TwistyTilePointLimits final {
public:
    TwistyTilePointLimits
        (const Size2 & twisty_size, const Vector2I & tile_pos, Real t_value);

    Vector2 spine_point_on_tile() const { return m_tile_spine; }

    Vector spine_point_in_3d() const { return m_spine; }

    Vector2 edge_point_on_tile() const { return m_tile_edge; }

    Vector edge_point_in_3d() const { return m_edge; }

private:
    Vector2 m_tile_spine, m_tile_edge;
    Vector m_spine, m_edge;
};

Vector to_twisty_offset(const Real & radius, Real t);

Vector to_twisty_spine(const Size2 & twisty_size, Real t);

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
     const Size2 & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end);

ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2 & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment);

ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2 & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTilePointLimits & low_lims,
     const TwistyTilePointLimits & high_lims);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const Size2 & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTilePointLimits & low_lims,
     const TwistyTilePointLimits & high_lims);

class NorthSouthTwistTileGroup final : public TwistTileGroup {
public:
    static constexpr const Real k_breaks_per_segment = 2;

    void load
        (const Rectangle &, const TileSetXmlGrid &, Platform &) final;

    void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const final;

private:
    Grid<SharedPtr<const RenderModel>> m_group_models;
};

class TwistLoopTile final : public ProducableTile {
public:
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
};
