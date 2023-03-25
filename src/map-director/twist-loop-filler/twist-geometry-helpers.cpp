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

#include "twist-geometry-helpers.hpp"

constexpr const auto k_twisty_origin = TwistyStripRadii::k_twisty_origin;

namespace {

using namespace cul::exceptions_abbr;
using cul::size_of, cul::top_left_of;

} // end of <anonymous> namespace

Optional<TwistyTileTValueLimits> TwistyTileTValueLimits::find
    (const Size2I & twisty_size, const Vector2I & tile_pos)
{
    using RadiiLims = TwistyStripRadii;
    TwistyTileTValueRange range{twisty_size.height, tile_pos.y};
    auto low_radii  = RadiiLims::find(twisty_size.width, tile_pos.x, range.low_t ());
    auto high_radii = RadiiLims::find(twisty_size.width, tile_pos.x, range.high_t());
    if (!low_radii && !high_radii) {
        return {};
    }
    // it's possible for there to be two intersections
    // it maybe good enough to just use the nearest
    auto closest_alternate = make_closest_alternate(twisty_size, tile_pos);

    return TwistyTileTValueLimits
        {low_radii  ? range.low_t () : closest_alternate(range.low_t ()),
         high_radii ? range.high_t() : closest_alternate(range.high_t())};
}


Optional<Real> TwistyTileTValueLimits::intersecting_t_value
    (const Size2I & twisty_size, int strip_x,
     const TwistyTileTValueRange & t_range)
{
    auto edge_x = TwistyStripSpineOffsets::edge_x_offset
        (twisty_size.width, strip_x);
    auto normalized_offset_from_spine =
        // direction relative to spine
        // normalize(strip_x - Real(twisty_size.width) / 2)
        // distance of the edge from the spine normalized to [0 1]
        //*
        (edge_x*2 / twisty_size.width);
    // find that first possible t
    auto t_sol_0 = std::acos(normalized_offset_from_spine);
    auto t_sol_1 = 1 - t_sol_0;
    if (t_range.contains(t_sol_0))
        return t_sol_0;
    if (t_range.contains(t_sol_1))
        return t_sol_1;
    return {};
}

Vector to_twisty_offset(const Real & radius, Real t) {
    auto theta = t*2*k_pi;
    return Vector
        {radius*Real(0.5)*std::cos(theta),
         radius*Real(0.5)*std::sin(theta), 0};
}

Vector to_twisty_spine(const Size2 & twisty_size, Real t) {
    return   k_twisty_origin
           + Vector{twisty_size.width*Real(0.5), 0, t*twisty_size.height*Real(0.5)};
}

std::vector<Real> find_unavoidable_t_breaks_for_twisty
    (const Size2I & twisty_size)
{
    std::vector<Real> tbreaks;
    for (int x = 0; x != twisty_size.width ; ++x) {
    for (int y = 0; y != twisty_size.height; ++y) {
        auto lims = TwistyTileTValueLimits::find(twisty_size, Vector2I{x, y});
        if (!lims) continue;
        tbreaks.push_back(lims->low_t_limit ());
        tbreaks.push_back(lims->high_t_limit());
    }}
    std::sort(tbreaks.begin(), tbreaks.end());
    auto uend = std::unique(tbreaks.begin(), tbreaks.end(), [](Real lhs, Real rhs) {
        return are_very_close(lhs, rhs);
    });
    tbreaks.erase(uend, tbreaks.end());
    return tbreaks;
}


std::vector<Real> verify_ordered
    (const char * caller, std::vector<Real> && reals)
{
    if (std::is_sorted(reals.begin(), reals.end())) return std::move(reals);
    throw InvArg{std::string{caller} + ": requires ordered tbreaks"};
}

std::vector<Real> verify_at_least_two
    (const char * caller, std::vector<Real> && reals)
{
    if (reals.size() >= 2) return std::move(reals);
    throw InvArg{std::string{caller} + ": requires at least two tbreaks"};
}

std::vector<Real> verify_all_within_zero_to_one
    (const char * caller, std::vector<Real> && reals)
{
    if (std::all_of
        (reals.begin(), reals.end(), [](Real x) { return x >= 0 && x <= 1; }))
    { return std::move(reals); }
    throw InvArg{std::string{caller} + ": all tbreaks must be in [0 1]"};
}

std::vector<Real> pad_t_breaks_until_target
    (int target_number_of_breaks, std::vector<Real> && unavoidable_breaks)
{
    using namespace cul::exceptions_abbr;
    // unavoidable breaks must be:
    // - sorted
    // - all within [0 1]
    // - at least two elements
    static constexpr const auto k_fn_name = "pad_t_breaks_until_target";
    unavoidable_breaks =
        verify_ordered(k_fn_name,
            verify_all_within_zero_to_one(k_fn_name,
                verify_at_least_two(k_fn_name, std::move(unavoidable_breaks))));
    if (int(unavoidable_breaks.size()) > target_number_of_breaks) {
        return std::move(unavoidable_breaks);
    }
    // insert break, then all other iterators go bad...
    // I don't know how to avoid a O(n*m) implementation
    // naive probably enough anyhow...
    while (--target_number_of_breaks) {
        auto insert_pos = unavoidable_breaks.begin();
        // find the largets gap
        for (auto itr = unavoidable_breaks.begin();
             itr != unavoidable_breaks.end() - 1; ++itr)
        {
            auto this_gap = magnitude((*itr) - (*itr + 1));
            auto cand_gap = magnitude((*insert_pos) - (*insert_pos + 1));
            if (this_gap < cand_gap) {
                insert_pos = itr;
            }
        }
        auto mid = (*insert_pos + *(insert_pos + 1)) / 2;
        unavoidable_breaks.insert(insert_pos, mid);
    }
    return std::move(unavoidable_breaks);
}

ViewGrid<VertexTriangle> make_twisty_geometry_for
    (const Size2I & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment)
{
    int target_breaks = int(std::round(breaks_per_segment*twisty_size.height));
    auto t_breaks = pad_t_breaks_until_target
        (target_breaks, find_unavoidable_t_breaks_for_twisty(twisty_size));

    ViewGridInserter<VertexTriangle> triangle_inserter{Size2I{twisty_size}};
    for (; !triangle_inserter.filled(); triangle_inserter.advance()) {
#       if 0
        TwistyTileTValueLimits tile_lims{twisty_size, triangle_inserter.position()};
#       endif
        auto tile_lims = TwistyTileTValueLimits::find
            (twisty_size, triangle_inserter.position());
        if (!tile_lims)
            { continue; }
        // find all t breaks for this tile
        auto t_break_start = std::lower_bound
            (t_breaks.begin(), t_breaks.end(), tile_lims->low_t_limit());
        auto t_break_last = std::lower_bound
            (t_break_start, t_breaks.end(), tile_lims->high_t_limit());
        auto t_break_end = ((t_break_last == t_breaks.end()) ? 0 : 1) + t_break_start;
        insert_twisty_geometry_into
            (triangle_inserter, twisty_size, txadapter,
             t_break_start, t_break_end               );
    }
    return ViewGrid<VertexTriangle>{std::move(triangle_inserter)};
}

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTileEdgePoints & low_points,
     const TwistyTileEdgePoints & high_points);

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const Size2I & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end)
{
    if (t_breaks_end - t_breaks_beg < 2) return;

    auto last = t_breaks_beg;
    for (auto t : View{t_breaks_beg + 1, t_breaks_end}) {
        TwistyTileEdgePoints low_points {twisty_size, inserter.position().x, *last};
        TwistyTileEdgePoints high_points{twisty_size, inserter.position().x, t};
        insert_twisty_geometry_into(inserter, txadapter, low_points, high_points);
        ++last;
    }
}

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTileEdgePoints & low_points,
     const TwistyTileEdgePoints & high_points)
{
    using PointPair = TwistyTileEdgePoints::PointPair;
    TwistyTilePoints points{low_points, high_points};
    auto to_vertex = [&txadapter] (const PointPair & point)
        { return Vertex{point.in_3d(), txadapter(point.on_tile())}; };
    if (points.count() > 2) {
        inserter.push(VertexTriangle
            {to_vertex(points[0]), to_vertex(points[1]), to_vertex(points[2])});
    }
    if (points.count() > 3) {
        inserter.push(VertexTriangle
            {to_vertex(points[2]), to_vertex(points[3]), to_vertex(points[0])});
    }
}
