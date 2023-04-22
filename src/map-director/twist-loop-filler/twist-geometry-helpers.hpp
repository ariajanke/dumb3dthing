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

#include "../../RenderModel.hpp"
#include "../ViewGrid.hpp"

enum class TwistDirection {
    left, right
};

enum class TwistPathDirection {
    north_south,
    east_west
};

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
    // interface is not correct, there's an unused parameter
    TwistyTileTValueRange
        (int twisty_height, int tile_pos_y):
        m_low_t (Real(tile_pos_y    ) / twisty_height),
        m_high_t(Real(tile_pos_y + 1) / twisty_height) {}

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
    using IntersectingTValueFunc = Optional<Real> (*)
        (const Size2I &, int, const TwistyTileTValueRange &);

    template <IntersectingTValueFunc intersecting_t_value_f>
    class ClosestAlternateFinder final {
    public:
        // tile location x maybe two different values, this should not be
        // possible
        ClosestAlternateFinder
            (const Size2I & twisty_size, const Vector2I & tile_pos)
        {
            TwistyTileTValueRange range{twisty_size.height, tile_pos.y};
            auto intersecting_t_value_ = [twisty_size, range] (int strip_x)
                { return intersecting_t_value_f(twisty_size, strip_x, range); };
            m_intersect_low  = intersecting_t_value_(tile_pos.x    );
            m_intersect_high = intersecting_t_value_(tile_pos.x + 1);
        }

        Real operator () (Real t_value) const {
            if (m_intersect_low && m_intersect_high) {
                if (  magnitude(*m_intersect_low  - t_value)
                    < magnitude(*m_intersect_high - t_value))
                { return *m_intersect_low; }
                return *m_intersect_high;
            }
            if (m_intersect_low ) return *m_intersect_low;
            if (m_intersect_high) return *m_intersect_high;
            throw BadBranchException{__LINE__, __FILE__};
        }

    private:
        Optional<Real> m_intersect_low, m_intersect_high;
    };

    static Optional<TwistyTileTValueLimits> find
        (const Size2I & twisty_size, const Vector2I & tile_pos);

    static Optional<Real> intersecting_t_value
        (const Size2I & twisty_size, int strip_x,
         const TwistyTileTValueRange &);

    template <typename ... Types>
    static auto make_closest_alternate(Types &&... args) {
        return ClosestAlternateFinder<intersecting_t_value>
            (std::forward<Types>(args)...);
    }

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
        return TwistyStripRadii
            {magnitude(offsets->spine() / cos_t),
             magnitude(offsets->edge () / cos_t)};
    }

    // these should only be positive reals
    Real spine() const { return m_spine; }

    Real edge() const { return m_edge; }

private:
    TwistyStripRadii(Real spine_, Real edge_):
        m_spine(verify_non_negative_real("TwistyStripRadii", spine_)),
        m_edge (verify_non_negative_real("TwistyStripRadii", edge_ )) {}

    static Real verify_non_negative_real(const char * caller, Real x) {
        if (x >= 0) return x;
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
        auto itr = m_elements.begin();
        *itr++ = PointPair
            {Vector2{offsets->edge() + spine_pos, y_pos}, dir*radii->edge()};
        if (!are_very_close(radii->edge(), radii->spine())) {
            *itr++ = PointPair
                {Vector2{offsets->spine() + spine_pos, y_pos},
                 dir*radii->spine()};

        }
        m_count = itr - m_elements.begin();
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

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTileEdgePoints & low_points,
     const TwistyTileEdgePoints & high_points);
