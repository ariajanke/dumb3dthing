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

#include "WallTileFactory.hpp"

namespace {

using Triangle = TriangleSegment;
using SplitOpt = WallTileFactoryBase::SplitOpt;
using KnownCorners = WallTileFactoryBase::KnownCorners;

constexpr const auto k_bottom_only = SplitOpt::k_bottom_only;
constexpr const auto k_top_only    = SplitOpt::k_top_only;
constexpr const auto k_wall_only   = SplitOpt::k_wall_only;

void east_west_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

// should handle division == 0, == 1
// everything around
// {-0.5, x,  0.5}, {0.5, x,  0.5}
// {-0.5, x, -0.5}, {0.5, x, -0.5}
void north_south_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

void south_north_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

void west_east_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f);

void northwest_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southwest_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void northeast_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southeast_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void northwest_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southwest_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void northeast_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southeast_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

/* private */ bool TwoWayWallTileFactory::is_okay_wall_direction
    (CardinalDirection dir) const noexcept
{
    using Cd = CardinalDirection;
    constexpr static std::array k_list = { Cd::n, Cd::e, Cd::s, Cd::w };
    return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
}

/* private */ KnownCorners TwoWayWallTileFactory::make_known_corners() const {
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

/* private */ void TwoWayWallTileFactory::make_triangles
    (const Slopes & elvs, Real thershold, SplitOpt split_opt,
     const TriangleAdder & add_triangle) const
{
    auto make_triangles = [this] {
        using Cd = CardinalDirection;
        switch (direction()) {
        case Cd::n : return north_south_split;
        case Cd::s : return south_north_split;
        case Cd::e : return east_west_split  ;
        case Cd::w : return west_east_split  ;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    } ();

    make_triangles(elvs.nw, elvs.ne, elvs.sw, elvs.se, thershold,
                   split_opt, add_triangle);
}

// ----------------------------------------------------------------------------

bool CornerWallTileFactory::is_okay_wall_direction
    (CardinalDirection dir) const noexcept
{
    using Cd = CardinalDirection;
    constexpr static std::array k_list = { Cd::ne, Cd::nw, Cd::se, Cd::sw };
    return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
}

// ----------------------------------------------------------------------------

/* private */ InWallTileFactory::KnownCorners
    InWallTileFactory::make_known_corners() const
{
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

/* private */ void InWallTileFactory::make_triangles
    (const Slopes & elvs, Real thershold, SplitOpt split_opt,
     const TriangleAdder & add_triangle) const
{
    auto make_triangles = [this] {
        using Cd = CardinalDirection;
        switch (direction()) {
        case Cd::ne: return northeast_in_corner_split;
        case Cd::nw: return northwest_in_corner_split;
        case Cd::se: return southeast_in_corner_split;
        case Cd::sw: return southwest_in_corner_split;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    } ();

    make_triangles(
        elvs.nw, elvs.ne, elvs.sw, elvs.se, thershold, split_opt,
        add_triangle);
}

// ----------------------------------------------------------------------------

KnownCorners OutWallTileFactory::make_known_corners() const {
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

/* private */ void OutWallTileFactory::make_triangles
    (const Slopes & elvs, Real thershold, SplitOpt split_opt,
     const TriangleAdder & add_triangle) const
{
    auto make_triangles = [this] {
        using Cd = CardinalDirection;
        switch (direction()) {
        case Cd::ne: return northeast_out_corner_split;
        case Cd::nw: return northwest_out_corner_split;
        case Cd::se: return southeast_out_corner_split;
        case Cd::sw: return southwest_out_corner_split;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    } ();

    make_triangles(elvs.nw, elvs.ne, elvs.sw, elvs.se, thershold,
                   split_opt, add_triangle);
}

namespace { // ----------------------------------------------------------------

template <typename Func>
void make_linear_triangle_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     Real step, Func && f);

template <typename TransformingFunc, typename PassOnFunc>
/* <! auto breaks BFS ordering !> */ auto make_triangle_transformer
    (TransformingFunc && tf, const PassOnFunc & pf)
{
    return TriangleAdder::make([tf = std::move(tf), &pf](const Triangle & tri)
        { pf(Triangle{tf(tri.point_a()), tf(tri.point_b()), tf(tri.point_c())}); });
}

Vector xz_swap_roles(const Vector & r) { return Vector{r.z, r.y, r.x}; };

Vector invert_z(const Vector & r) { return Vector{r.x, r.y, -r.z}; }

Vector invert_x(const Vector & r) { return Vector{-r.x, r.y, r.z}; }

Vector invert_xz(const Vector & r) { return Vector{-r.x, r.y, -r.z}; }

void east_west_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f)
{
    // simply switch roles
    // east <-> north
    // west <-> south
    north_south_split
        (south_east_y, north_east_y, south_west_y, north_west_y,
         division_x, opt,
         make_triangle_transformer(xz_swap_roles, f));
}

// this is a bit different, presently it makes no assumption on which is the
// floor, and which is the top
// I think I may make this function more specialized
// north is bottom, south is top
void north_south_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f)
{
    // late division, less top space, early division... more
    // in all cases, southerns must be reals
    // in all but top case, northerns must also be reals
    using cul::is_real;
    if (!is_real(south_west_y) || !is_real(south_east_y)) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    }
    if (   (opt & ~k_top_only)
        && (!is_real(north_west_y) || !is_real(north_east_y)))
    {
        throw InvalidArgument
            {"north_south_split: Northern elevations must be real numbers in "
             "top cases"};
    }
    if (division_z < -0.5 || division_z > 0.5) {
        throw InvalidArgument
            {"north_south_split: division must be in [0.5 0.5]"};
    }
    if (   (opt & ~k_top_only)
        && (south_west_y < north_west_y || south_east_y < north_east_y))
    {
        throw InvalidArgument
            {"north_south_split: method was designed assuming south is the top"};
    }

    const Vector div_nw{-0.5, north_west_y, -division_z};
    const Vector div_sw{-0.5, south_west_y, -division_z};

    const Vector div_ne{ 0.5, north_east_y, -division_z};
    const Vector div_se{ 0.5, south_east_y, -division_z};

    if (opt & k_bottom_only) {
        Vector nw{-0.5, north_west_y, 0.5};
        Vector ne{ 0.5, north_east_y, 0.5};
        make_linear_triangle_strip(nw, div_nw, ne, div_ne, 1, f);
    }

    if (opt & k_top_only) {
        Vector sw{-0.5, south_west_y, -0.5};
        Vector se{ 0.5, south_east_y, -0.5};
        make_linear_triangle_strip(div_sw, sw, div_se, se, 1, f);
    }

    // We should only skip triangles along the wall if
    // there's no elevation difference to cover
    if (opt & k_wall_only) {
        // both sets of y values' directions must be the same
        assert((north_east_y - north_west_y)*(south_east_y - south_west_y) >= 0);
        make_linear_triangle_strip(div_nw, div_sw, div_ne, div_se, 1, f);
    }
}

void south_north_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f)
{
    north_south_split
        (south_west_y, south_east_y, north_west_y, north_east_y, division_z, opt,
         make_triangle_transformer(invert_z, f));
}

void west_east_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f)
{
    east_west_split
        (north_east_y, north_west_y, south_east_y, south_west_y, division_x, opt,
         make_triangle_transformer(invert_x, f));
}

void northwest_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    // only one maybe non-real
    using cul::is_real;
    bool can_only_do_top =
            is_real(north_east_y) &&  is_real(north_west_y)
        &&  is_real(south_west_y) && !is_real(south_east_y);
    bool are_all_real =
           is_real(north_east_y) && is_real(north_west_y)
        && is_real(south_west_y) && is_real(south_east_y);
    if (   (opt & ~k_top_only)
        && can_only_do_top)
    {
        throw InvalidArgument{"northwest_in_corner_split: "};
    }
    if (   are_all_real
        && (   south_east_y < north_west_y || south_east_y < north_east_y
            || south_east_y < south_west_y))
    {
        throw InvalidArgument
            {"northwest_in_corner_split: south_east_y is assumed to be the "
             "top's elevation, method not explicitly written to handle south "
             "east *not* being the top"};
    }
    // amount of top space when should follow out corner:
    // "late division, less top space, early division... more"
    // elevation assignments are different than out corners
    // the top "flat's" depth/width remain equal regardless where the division
    // is placed
    // divisions are place on different lines, that is between:
    // - nw and ne
    // - nw and sw
    // the "control" points are:
    Vector nw          {       -0.5, north_west_y,          0.5};
    Vector center_floor{division_xz, north_west_y, -division_xz};
    Vector center_top  {division_xz, south_east_y, -division_xz};
    Vector nw_ne_floor {division_xz, north_west_y,          0.5};
    Vector nw_ne_top   {division_xz, north_east_y,          0.5};
    Vector nw_sw_floor {       -0.5, north_west_y, -division_xz};
    Vector nw_sw_top   {       -0.5, north_east_y, -division_xz};
    Vector se          {        0.5, south_east_y,         -0.5};
    Vector sw          {       -0.5, south_west_y,         -0.5};
    Vector ne          {        0.5, north_east_y,          0.5};

    // they're so f***ing symmetric there's gotta be a way to merge
    // implementations
    if (opt & k_top_only && !are_very_close(division_xz, 0.5)) {
        // top is the relatively more complex shape here
        // four triangles
        f(Triangle{ne, center_top, se});
        f(Triangle{sw, center_top, se});
        if (!are_very_close(division_xz, -0.5)) {
            f(Triangle{ne, center_top, nw_ne_top});
            f(Triangle{sw, center_top, nw_sw_top});
        }
    }

    if (opt & k_bottom_only && !are_very_close(division_xz, -0.5)) {
        f(Triangle{nw, nw_ne_floor, center_floor});
        f(Triangle{nw, nw_sw_floor, center_floor});
    }

    if (opt & k_wall_only && !are_very_close(division_xz, 0.5)) {
        make_linear_triangle_strip(nw_ne_top, nw_ne_floor, center_top, center_floor, 1, f);
        make_linear_triangle_strip(nw_sw_top, nw_sw_floor, center_top, center_floor, 1, f);
    }
}

void southwest_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_in_corner_split
        (south_west_y, south_east_y, north_west_y, north_east_y, division_xz,
         opt, make_triangle_transformer(invert_z, f));
}

void northeast_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_in_corner_split
        (north_east_y, north_west_y, south_east_y, south_west_y, division_xz,
         opt, make_triangle_transformer(invert_x, f));
}

void southeast_in_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_in_corner_split
        (south_east_y, south_west_y, north_east_y, north_west_y, division_xz,
         opt, make_triangle_transformer(invert_xz, f));
}

void northwest_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    // se as only Real...
    using cul::is_real;
    bool can_only_do_top =
           !is_real(north_east_y) && !is_real(north_west_y)
        && !is_real(south_west_y) &&  is_real(south_east_y);
    bool are_all_real =
           is_real(north_east_y) && is_real(north_west_y)
        && is_real(south_west_y) && is_real(south_east_y);
    if (   (opt & ~k_top_only)
        && can_only_do_top)
    {
        throw InvalidArgument{"northwest_out_corner_split: "};
    }
    if (   are_all_real
        && (   south_east_y < north_west_y || south_east_y < north_east_y
            || south_east_y < south_west_y))
    {
        throw InvalidArgument
            {"northwest_out_corner_split: south_east_y is assumed to be the "
             "top's elevation, method not explicitly written to handle south "
             "east *not* being the top"};
    }
    // late division, less top space, early division... more
    // the top "flat's" depth/width remain equal regardless where the division
    // is placed
    // divisions are placed between:
    // - ne, and se
    // - sw, and se
    // the "control" points are:
    // nw corner, floor, top
    Vector nw_corner{       -0.5, north_west_y,          0.5};
    Vector nw_floor {division_xz, north_west_y, -division_xz};
    Vector nw_top   {division_xz, south_east_y, -division_xz};
    // se
    Vector se       {        0.5, south_east_y,         -0.5};
    // ne corner, floor, top
    Vector ne_corner{        0.5, north_east_y,          0.5};
    Vector ne_floor {        0.5, north_east_y, -division_xz};
    Vector ne_top   {        0.5, south_east_y, -division_xz};
    // sw corner, floor, top
    Vector sw_corner{       -0.5, south_west_y,         -0.5};
    Vector sw_floor {division_xz, south_west_y,         -0.5};
    Vector sw_top   {division_xz, south_east_y,         -0.5};

    // both top triangles should come together or not at all
    // there is only one condition where no tops are generated:
    // - division is ~0.5
    if (opt & k_top_only && !are_very_close(division_xz, 0.5)) {
        f(Triangle{nw_top, ne_top, se});
        f(Triangle{nw_top, sw_top, se});
    }

    // four triangles for the bottom,
    // all triangles should come together, or not at all
    if (opt & k_bottom_only && !are_very_close(division_xz, -0.5)) {
        f(Triangle{nw_corner, ne_corner, ne_floor});
        // this constructor should be throwing when div ~= 0.5
        if (!are_very_close(division_xz, 0.5)) {
            f(Triangle{nw_corner, nw_floor , ne_floor});

            f(Triangle{nw_corner, nw_floor , sw_floor});
        }
        f(Triangle{nw_corner, sw_corner, sw_floor});
    }

    if (opt & k_wall_only) {
        make_linear_triangle_strip(nw_top, nw_floor, ne_top, ne_floor, 1, f);
        make_linear_triangle_strip(nw_top, nw_floor, sw_top, sw_floor, 1, f);
    }
}

// can just exploit symmetry to implement the rest
void southwest_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_out_corner_split
        (south_west_y, south_east_y, north_west_y, north_east_y, division_xz,
         opt, make_triangle_transformer(invert_z, f));
}

void northeast_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_out_corner_split
        (north_east_y, north_west_y, south_east_y, south_west_y, division_xz,
         opt, make_triangle_transformer(invert_x, f));
}

void southeast_out_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    northwest_out_corner_split
        (south_east_y, south_west_y, north_east_y, north_west_y, division_xz,
         opt, make_triangle_transformer(invert_xz, f));
}

// ----------------------------------------------------------------------------

/* <! auto breaks BFS ordering !> */ auto
    make_get_next_for_dir_split_v
    (Vector end, Vector step)
{
    return [end, step] (Vector east_itr) {
        auto cand_next = east_itr + step;
        if (are_very_close(cand_next, end)) return cand_next;

        if (are_very_close(normalize(end - east_itr ),
                           normalize(end - cand_next)))
        { return cand_next; }
        return end;
    };
}

/* <! auto breaks BFS ordering !> */ auto make_step_factory(Real step) {
    return [step](const Vector & start, const Vector & last) {
        auto diff = last - start;
        if (are_very_close(diff, Vector{})) return Vector{};
        return step*normalize(diff);
    };
}

template <typename Func>
void make_linear_triangle_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     Real step, Func && f)
{
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }

    const auto make_step = make_step_factory(step);

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
            f(Triangle{itr_a, itr_b, new_a});
        if (!are_very_close(new_a, new_b))
            f(Triangle{itr_b, new_a, new_b});
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

        f(Triangle{itr_a, itr_b, a_last});
        return;
    } else {
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
            f(Triangle{itr_a, b_last, a_last});
            return;
        } else if (!are_very_close(itr_b, b_last)) {
            // must exclude itr_a
            f(Triangle{itr_b, a_last, b_last});
            return;
        }
    }
    throw BadBranchException{__LINE__, __FILE__};
}

} // end of <anonymous> namespace
