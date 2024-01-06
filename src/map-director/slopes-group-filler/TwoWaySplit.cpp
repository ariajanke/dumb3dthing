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

#include "TwoWaySplit.hpp"
#include "OutWallTilesetTileN.hpp"

#include "../../TriangleSegment.hpp"

namespace {

using Triangle = TriangleSegment;

class NorthGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        NorthSouthSplit nss{elevations, division_z};
        with_split_callback(nss);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             {},
             elevations.south_west(),
             elevations.south_east()};
    }
};

class SouthGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        SouthNorthSplit sns{elevations, division_z};
        with_split_callback(sns);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             elevations.north_west(),
             {},
             {}};
    }
};

class EastGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        EastWestSplit ews{elevations, division_z};
        with_split_callback(ews);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             elevations.north_west(),
             elevations.south_west(),
             {}};
    }
};

class WestGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        WestEastSplit wes{elevations, division_z};
        with_split_callback(wes);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             {},
             {},
             elevations.south_east()};
    }
};

StripVertex::StripSide other_side_of(StripVertex::StripSide side) {
    using Side = StripVertex::StripSide;
    switch (side) {
    case Side::a: return Side::b;
    case Side::b: return Side::a;
    default: break;
    }
    throw InvalidArgument{"bad argument"};
}

} // end of <anonymous> namespace
#if 0
void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using Triangle = TriangleSegment;
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }

    const auto make_step = make_step_factory(steps_count);

    auto itr_a = a_start;
    const auto next_a = make_get_next_for_dir_split_v(
        a_last, make_step(a_start, a_last));

    auto itr_b = b_start;
    const auto next_b = make_get_next_for_dir_split_v(
        b_last, make_step(b_start, b_last));

    while (   !are_very_close(itr_a, a_last)
           && !are_very_close(itr_b, b_last))
    {
        const auto new_a = next_a(itr_a);
        const auto new_b = next_b(itr_b);
        if (!are_very_close(itr_a, itr_b))
            add_triangle(Triangle{itr_a, itr_b, new_a});
        if (!are_very_close(new_a, new_b))
            add_triangle(Triangle{itr_b, new_a, new_b});
        itr_a = new_a;
        itr_b = new_b;
    }

    // at this point we are going to generate at most one triangle
    if (are_very_close(b_last, a_last)) {
        // here we're down to three points
        // there is only one possible triangle
        if (   are_very_close(itr_a, a_last)
            || are_very_close(itr_a, itr_b))
        {
            // take either being true:
            // in the best case: a line, so nothing
            return;
        }

        add_triangle(Triangle{itr_a, itr_b, a_last});
        return;
    }
    // a reminder from above
    assert(   are_very_close(itr_a, a_last)
           || are_very_close(itr_b, b_last));

    // here we still haven't ruled any points out
    if (   are_very_close(itr_a, itr_b)
        || (   are_very_close(itr_a, a_last)
            && are_very_close(itr_b, b_last)))
    {
        // either are okay, as they are "the same" pt
        return;
    } else if (!are_very_close(itr_a, a_last)) {
        // must exclude itr_b
        add_triangle(Triangle{itr_a, b_last, a_last});
        return;
    } else if (!are_very_close(itr_b, b_last)) {
        // must exclude itr_a
        add_triangle(Triangle{itr_b, a_last, b_last});
        return;
    }
}
#endif

StripTriangle::StripTriangle
    (const StripVertex & a,
     const StripVertex & b,
     const StripVertex & c):
    m_a(a),
    m_b(b),
    m_c(c) {}

TriangleSegment StripTriangle::to_triangle_segment() const {
    return TriangleSegment{m_a.point, m_b.point, m_c.point};
}

StripVertex StripTriangle::vertex_a() const { return m_a; }

StripVertex StripTriangle::vertex_b() const { return m_b; }

StripVertex StripTriangle::vertex_c() const { return m_c; }
#if 0
class StripIteration final {
public:
    using StripSide = StripVertex::StripSide;

    StripIteration
        (const Vector & start,
         const Vector & last,
         int steps_count,
         StripSide side,
         int current_step = 0):
        m_steps_count(are_very_close(start, last) ? Optional<int>{} : steps_count),
        m_current_step(current_step),
        m_start(start),
        m_last(last),
        m_side(side) {}

    StripTriangle triangle_with_next(const StripVertex & vtx) const {
        if (!m_steps_count) {
            throw RuntimeError{"has only one point"};
        }
        return StripTriangle{vertex(), vtx, next_vertex()};
    }

    StripVertex vertex() const
        { return vertex_at(m_current_step); }

    StripVertex vertex_with(StripSide side) const
        { return vertex_at_with_side(m_current_step, side); }

    StripVertex next_vertex() const
        { return vertex_at(m_current_step + 1); }

    StripIteration next() const {
        return StripIteration
            {m_current_step + 1, m_steps_count, m_start, m_last, m_side};
    }

private:
    StripIteration
        (int current_step,
         Optional<int> steps_count,
         const Vector & start,
         const Vector & last,
         StripSide side):
        m_steps_count(steps_count),
        m_current_step(current_step),
        m_start(start),
        m_last(last),
        m_side(side) {}

    Optional<Real> strip_position() const
        { return strip_position_at(m_current_step); }

    Optional<Real> strip_position_at(int step) const {
        if (m_steps_count)
            return Real(step) / Real(*m_steps_count);
        return {};
    }

    StripVertex vertex_at(int step) const {
        return vertex_at_with_side(step, m_side);
    }

    StripVertex vertex_at_with_side(int step, StripSide side) const {
        return StripVertex
            {point_at(step),
             strip_position_at(step),
             side};
    }

    Vector point_at(int step) const {
        if (auto t = strip_position_at(step)) {
            return m_start*(1 - *t) + m_last*(*t);
        }
        return m_start;
    }

    Optional<int> m_steps_count = 1;
    int m_current_step = 0;
    Vector m_start;
    Vector m_last;
    StripSide m_side;
};
#endif

/* private */ void LinearStripTriangleCollection::triangle_strip
    (const Vector & a_point,
     const Vector & b_start,
     const Vector & b_last,
     StripVertex::StripSide a_side,
     int steps_count)
{
    if (are_very_close(b_start, b_last)) {
        throw InvalidArgument{""};
    }
    auto b_side_pt = [b_start, b_last] (Real t)
        { return b_start*(1 - t) + b_last*t; };
    auto b_side = other_side_of(a_side);
    for (int i = 0; i != steps_count; ++i) {
        Real t = Real(i) / Real(steps_count);
        Real next_t = Real(i + 1) / Real(steps_count);
        add_triangle(StripTriangle
            {StripVertex{a_point, {}, a_side},
             StripVertex{b_side_pt(t     ), t     , b_side},
             StripVertex{b_side_pt(next_t), next_t, b_side}});
    }
}

#if 1
void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using StripSide = StripVertex::StripSide;
    using Triangle = TriangleSegment;
    if (steps_count < 0) {
        throw InvalidArgument{"steps_count must be a non-negative integer"};
    }

    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    {
        return;
    }
    // atempting to generate a one dimensional line
    else if (   are_very_close(a_start, b_start)
             && are_very_close(a_last , b_last ))
    {
        return;
    } else if (steps_count == 0) {
        return;
    } else if (are_very_close(a_start, a_last)) {
        return triangle_strip(a_start, b_start, b_last, StripSide::a, steps_count);
    } else if (are_very_close(b_start, b_last)) {
        return triangle_strip(b_start, a_start, a_last, StripSide::b, steps_count);
    }

    auto a_side_pt = [a_start, a_last] (Real t)
        { return a_start*(1 - t) + a_last*t; };
    auto b_side_pt = [b_start, b_last] (Real t)
        { return b_start*(1 - t) + b_last*t; };
    auto normal_step = [this, &a_side_pt, &b_side_pt] (Real last, Real next) {

        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{b_side_pt(last), last, StripSide::b},
             StripVertex{b_side_pt(next), next, StripSide::b}});
        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{a_side_pt(next), next, StripSide::a},
             StripVertex{b_side_pt(next), next, StripSide::b}});
    };
    auto first_step =
        [this, &a_side_pt, &b_side_pt, &a_start, &b_start]
        (Real next)
    {
        assert(are_very_close(a_start, b_start));
        add_triangle(StripTriangle
            {StripVertex{a_start, 0, {}},
             StripVertex{a_side_pt(next), next, StripSide::a},
             StripVertex{b_side_pt(next), next, StripSide::b}});
    };
    int step = 0;
    Real step_size = 1 / Real(steps_count);
    auto get_last = [step_size, step] { return step*step_size; };
    auto get_next = [step_size, step] { return (step + 1)*step_size; };

    if (step == 0 && steps_count > 1) {
        if (are_very_close(a_start, b_start)) {
            first_step(get_next());
        } else {
            // normal first step
            normal_step(get_last(), get_next());
        }
        ++step;
    }

    for (; step < steps_count - 1; ++step) {
        normal_step(get_last(), get_next());
    }

    if (step == 0 && are_very_close(a_start, b_start)) {
        first_step(get_next());
    } else if (are_very_close(a_last, b_last)) {
        auto last = get_last();
        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{b_side_pt(last), last, StripSide::b},
             StripVertex{a_last, 1, StripSide::both}});
    } else {
        normal_step(get_last(), get_next());
    }
}
#else
void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using StripSide = StripVertex::StripSide;
    using Triangle = TriangleSegment;
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }
    // atempting to generate a one dimensional line
    if (   are_very_close(a_start, b_start)
        && are_very_close(a_last , b_last ))
    { return; }
    if (steps_count < 0) {
        throw InvalidArgument{"steps_count must be a non-negative integer"};
    }

    StripIteration a_side{a_start, a_last, steps_count, StripSide::a};
    StripIteration b_side{b_start, b_last, steps_count, StripSide::b};

    if (are_very_close(a_start, b_start)) {
        add_triangle(StripTriangle
            {a_side.vertex_with(StripSide::both),
             b_side.next_vertex(),
             a_side.next_vertex()});
        a_side = a_side.next();
        b_side = b_side.next();
    }

    while (true /* until both side at last? */) {
        assert(false);
        // do sides
    }

    if (are_very_close(a_last, b_last)) {
        add_triangle(StripTriangle
            {a_side.vertex_with(StripSide::both),
             b_side.next_vertex(),
             a_side.next_vertex()});
    }
}
#endif

// ----------------------------------------------------------------------------

/* static */ TwoWaySplit::GeometryGenerationStrategy &
    TwoWaySplit::choose_geometry_strategy
    (CardinalDirection direction)
{
    static NorthGenerationStrategy north_gen_strat;
    static SouthGenerationStrategy south_gen_strat;
    static EastGenerationStrategy  east_gen_strat;
    static WestGenerationStrategy  west_gen_strat;
    switch (direction) {
    case CardinalDirection::north: return north_gen_strat;
    case CardinalDirection::south: return south_gen_strat;
    case CardinalDirection::east : return east_gen_strat ;
    case CardinalDirection::west : return west_gen_strat ;
    default: break;
    }
    // should probably return nullptr or something
    // (as this is based on user data)
    throw InvalidArgument{"bad direction"};
}

/* static */ TwoWaySplit::GeometryGenerationStrategy &
    TwoWaySplit::choose_out_wall_strategy(CardinalDirection direction)
{
    static NorthWestOutWallGenerationStrategy nw_out_strat;
    static NorthEastOutWallGenerationStrategy ne_out_strat;
    static SouthWestOutWallGenerationStrategy sw_out_strat;
    static SouthEastOutWallGenerationStrategy se_out_strat;
    switch (direction) {
    case CardinalDirection::north_west: return nw_out_strat;
    case CardinalDirection::north_east: return ne_out_strat;
    case CardinalDirection::south_west: return sw_out_strat;
    case CardinalDirection::south_east: return se_out_strat;
    default: break;
    }
    throw InvalidArgument{"bad direction"};
}

/* static */ TwoWaySplit::GeometryGenerationStrategy &
    TwoWaySplit::choose_in_wall_strategy(CardinalDirection direction)
{
    static NorthWestInWallGenerationStrategy nw_in_strat;
    static NorthEastInWallGenerationStrategy ne_in_strat;
    static SouthWestInWallGenerationStrategy sw_in_strat;
    static SouthEastInWallGenerationStrategy se_in_strat;
    switch (direction) {
    case CardinalDirection::north_west: return nw_in_strat;
    case CardinalDirection::north_east: return ne_in_strat;
    case CardinalDirection::south_west: return sw_in_strat;
    case CardinalDirection::south_east: return se_in_strat;
    default: break;
    }
    throw InvalidArgument{"bad direction"};
}

// ----------------------------------------------------------------------------

NorthSouthSplit::NorthSouthSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    NorthSouthSplit
        (elevations.north_west(),
         elevations.north_east(),
         *elevations.south_west(),
         *elevations.south_east(),
         division_z) {}

NorthSouthSplit::NorthSouthSplit
    (Optional<Real> north_west_y,
     Optional<Real> north_east_y,
     Real south_west_y,
     Real south_east_y,
     Real division_z):
    m_div_nw(-0.5, north_west_y.value_or(k_inf), -division_z),
    m_div_sw(-0.5, south_west_y, -division_z),
    m_div_ne( 0.5, north_east_y.value_or(k_inf), -division_z),
    m_div_se( 0.5, south_east_y, -division_z)
{
    using cul::is_real;
#   if 0
    if (!is_real(*south_west_y) || !is_real(*south_east_y)) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    } else
#   endif
#   if 0
    if (!south_west_y || !south_east_y) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    }
#   endif
    if (division_z < -0.5 || division_z > 0.5) {
        throw InvalidArgument
            {"north_south_split: division must be in [-0.5 0.5]"};
    }
}

void NorthSouthSplit::make_top(LinearStripTriangleCollection & collection) const {
    Vector sw{-0.5, south_west_y(), -0.5};
    Vector se{ 0.5, south_east_y(), -0.5};
    collection.add_triangle(Triangle{sw      , se, m_div_sw}, cut_y);
    collection.add_triangle(Triangle{m_div_sw, se, m_div_se}, cut_y);
}

void NorthSouthSplit::make_bottom(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    Vector nw{-0.5, north_west_y(), 0.5};
    Vector ne{ 0.5, north_east_y(), 0.5};
    collection.add_triangle(Triangle{m_div_nw, m_div_ne, nw}, cut_y);
    collection.add_triangle(Triangle{      nw, m_div_ne, ne}, cut_y);
}

void NorthSouthSplit::make_wall(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    // both sets of y values' directions must be the same
    assert((north_east_y() - north_west_y())*
           (south_east_y() - south_west_y()) >= 0);
    collection.make_strip(m_div_nw, m_div_sw, m_div_ne, m_div_se, 1);
}

/* private */ void NorthSouthSplit::check_non_top_assumptions() const {
    using cul::is_real;
    if ((!is_real(north_west_y()) || !is_real(north_east_y()))) {
        throw InvalidArgument
            {"north_south_split: Northern elevations must be real numbers in "
             "top cases"};
    }
    if (south_west_y() < north_west_y() || south_east_y() < north_east_y()) {
        throw InvalidArgument
            {"north_south_split: method was designed assuming south is the top"};
    }
}

// ----------------------------------------------------------------------------

SouthNorthSplit::SouthNorthSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_ns_split(
        elevations.south_west(),
        elevations.south_east(),
        *elevations.north_west(),
        *elevations.north_east(),
        division_z) {}

// ----------------------------------------------------------------------------

WestEastSplit::WestEastSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_ns_split(
        elevations.north_west(),
        elevations.south_west(),
        *elevations.north_east(),
        *elevations.south_east(),
        division_z) {}

// ----------------------------------------------------------------------------

EastWestSplit::EastWestSplit
    (const TileCornerElevations & elevations,
     Real division_z):
     m_ns_split(
      elevations.south_east(),
      elevations.north_east(),
      *elevations.south_west(),
      *elevations.north_west(),
      division_z) {}
