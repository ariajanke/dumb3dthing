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

#include "TwistLoopGroupFiller.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

constexpr const auto k_twisty_origin = TwistyTileRadiiLimits::k_twisty_origin;

namespace {

using namespace cul::exceptions_abbr;
using cul::size_of, cul::top_left_of;

} // end of <anonymous> namespace

class TwistyTileTValueRange final {
public:
    TwistyTileTValueRange
        (const Size2 & twisty_size, const Vector2I & tile_pos):
        m_low_t (Real(tile_pos.x    ) / twisty_size.width),
        m_high_t(Real(tile_pos.x + 1) / twisty_size.width)
    {}

    Real low_t() const { return m_low_t; }

    Real high_t() const { return m_high_t; }

private:
    Real m_low_t;
    Real m_high_t;
};

TwistyTileTValueLimits::TwistyTileTValueLimits
    (const Size2 & twisty_size, const Vector2I & tile_pos)
{
    // need x positions for t = pos.x / width, (pos.x + 1) / width
    // if there's a cross, then I need to find that intersection
    TwistyTileTValueRange range{twisty_size, tile_pos};
    TwistyTileRadiiLimits low_radii{twisty_size, tile_pos, range.low_t()};
    TwistyTileRadiiLimits high_radii{twisty_size, tile_pos, range.high_t()};
    bool low_radii_regular = low_radii.edge_radius() > low_radii.spine_radius();
    bool high_radii_regular = high_radii.edge_radius() > high_radii.spine_radius();
    if (low_radii_regular && high_radii_regular) {
        m_low_t_limit = range.low_t();
        m_high_t_limit = range.high_t();
    } else if (!low_radii_regular && !high_radii_regular) {
        // just shove both points into one corner and call it good
        m_low_t_limit = m_high_t_limit = range.low_t();
    } else {
        // gotta find that t s.t. is the spine side's x
        auto sol = [] (const Size2 & twisty_size, const Vector2I & tile_pos) {
            TwistyTileTValueRange range{twisty_size, tile_pos};

            auto sol0 = std::acos(tile_pos.x - Real(twisty_size.width) / 2) / 2*k_pi;
            auto sol1 = sol0 + k_pi;
            assert(   (sol0 >= range.low_t() && sol0 <= range.high_t())
                   || (sol1 >= range.low_t() && sol1 <= range.high_t()));
            return (sol0 >= range.low_t() && sol0 <= range.high_t()) ? sol0 : sol1;
        } (twisty_size, tile_pos);

        if (low_radii_regular) {
            m_low_t_limit  = range.low_t();
            m_high_t_limit = sol;
        } else {
            m_low_t_limit  = sol;
            m_high_t_limit = range.high_t();
        }
    }

    // high and low theta
}

TwistyTileRadiiLimits::TwistyTileRadiiLimits
    (const Size2 & twisty_size, const Vector2I & tile_pos, Real t_value):
    m_t_value(t_value)
{

    // the needed radius to reach a point can be undefined if the twisty is
    // going straight up/down
    // spine point should come first, then edge point...

    // I have both points projected onto the plane of the twisty
    // now I try and limit them inside the bound's of that tile
    auto [low_edge_x, high_edge_x, low_dir] =
        low_high_x_edges_and_low_dir(twisty_size, t_value);

    // restrict x-ways
    auto [spine_side_x, edge_side_x] =
        spine_and_edge_side_x(twisty_size.width, tile_pos.x);

    // edge may slide into tile bounds, or use the tile bounds
    edge_side_x = std::max(low_edge_x, std::min(high_edge_x, edge_side_x));

    auto tan_theta = std::tan(t_value*2*k_pi);
    m_edge  = magnitude(edge_side_x *tan_theta);
    m_spine = magnitude(spine_side_x*tan_theta);
}

/* static */ Tuple<Real, Real, Real>
    TwistyTileRadiiLimits::low_high_x_edges_and_low_dir
    (const Size2 & twisty_size, Real t_value)
{
    auto spine_pt   = to_twisty_spine(Size2{twisty_size}, t_value);
    auto twisty_rad = Real(twisty_size.width) / 2;
    auto ex_edge_pt_a = spine_pt - to_twisty_offset(twisty_rad, t_value);
    auto ex_edge_pt_b = spine_pt + to_twisty_offset(twisty_rad, t_value);
    auto twisty_edge_a = cul::project_onto(ex_edge_pt_a, Vector{twisty_rad, 0, 0}).x;
    auto twisty_edge_b = cul::project_onto(ex_edge_pt_b, Vector{twisty_rad, 0, 0}).x;
    if (twisty_edge_a > twisty_edge_b) {
        return make_tuple(twisty_edge_b, twisty_edge_a, 1.);
    }
    return make_tuple(twisty_edge_a, twisty_edge_b, -1.);
}

/* static */ Tuple<Real, Real>
    TwistyTileRadiiLimits::spine_and_edge_side_x
    (Real twisty_width, Real tile_pos_x)
{
    auto tile_low_x = k_twisty_origin.x + tile_pos_x;
    auto tile_high_x = tile_low_x + 1;
    auto twisty_rad = twisty_width / 2;
    if (  magnitude(tile_low_x  - twisty_rad)
        > magnitude(tile_high_x - twisty_rad))
    {
        return make_tuple(tile_high_x, tile_low_x);
    }
    return make_tuple(tile_low_x, tile_high_x);
}

TwistyTilePointLimits::TwistyTilePointLimits
    (const Size2 & twisty_size, const Vector2I & tile_pos, Real t_value)
{
    TwistyTileRadiiLimits radii_lims{twisty_size, tile_pos, t_value};
    auto dir = to_twisty_offset(1, t_value);
    m_spine = dir*radii_lims.spine_radius();
    m_edge = dir*radii_lims.edge_radius();
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
    (const Size2 & twisty_size)
{
    std::vector<Real> tbreaks;
    for (int x = 0; x != twisty_size.width ; ++x) {
    for (int y = 0; y != twisty_size.height; ++y) {
        TwistyTileTValueLimits lims{twisty_size, Vector2I{x, y}};
        tbreaks.push_back(lims.low_t_limit ());
        tbreaks.push_back(lims.high_t_limit());
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
    (const Size2 & twisty_size, TwistDirection dir, TwistPathDirection path_dir,
     const TexturingAdapter & txadapter, Real breaks_per_segment)
{
    int target_breaks = int(std::round(breaks_per_segment*twisty_size.height));
    auto t_breaks = pad_t_breaks_until_target
        (target_breaks, find_unavoidable_t_breaks_for_twisty(twisty_size));

    ViewGridInserter<VertexTriangle> triangle_inserter{Size2I{twisty_size}};
    for (; !triangle_inserter.filled(); triangle_inserter.advance()) {
        TwistyTileTValueLimits tile_lims{twisty_size, triangle_inserter.position()};
        // find all t breaks for this tile
        auto t_break_start = std::lower_bound
            (t_breaks.begin(), t_breaks.end(), tile_lims.low_t_limit());
        auto t_break_last = std::lower_bound
            (t_break_start, t_breaks.end(), tile_lims.high_t_limit());
        auto t_break_end = ((t_break_last == t_breaks.end()) ? 0 : 1) + t_break_start;
        insert_twisty_geometry_into
            (triangle_inserter, twisty_size, txadapter,
             t_break_start, t_break_end               );
    }
    return ViewGrid<VertexTriangle>{std::move(triangle_inserter)};
}

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const Size2 & twisty_size, const TexturingAdapter & txadapter,
     std::vector<Real>::const_iterator t_breaks_beg,
     std::vector<Real>::const_iterator t_breaks_end)
{
    if (t_breaks_end - t_breaks_beg < 2) return;

    auto last = t_breaks_beg;
    for (auto t : View{t_breaks_beg + 1, t_breaks_end}) {
        TwistyTilePointLimits low_lims {twisty_size, inserter.position(), *last};
        TwistyTilePointLimits high_lims{twisty_size, inserter.position(), t};
        insert_twisty_geometry_into(inserter, txadapter, low_lims, high_lims);
        ++last;
    }
}

void insert_twisty_geometry_into
    (ViewGridInserter<VertexTriangle> & inserter,
     const TexturingAdapter & txadapter,
     const TwistyTilePointLimits & low_lims,
     const TwistyTilePointLimits & high_lims)
{
    if (!are_very_close(low_lims.edge_point_in_3d (),
                        low_lims.spine_point_in_3d()))
    {
        VertexTriangle triangle;
        triangle[0] = Vertex
            {low_lims.edge_point_in_3d(),
             txadapter(low_lims.edge_point_on_tile())};
        triangle[1] = Vertex
            {low_lims.spine_point_in_3d(),
             txadapter(low_lims.spine_point_on_tile())};
        // and one point on high lims... though I suppose it doesn't really
        // matter which point we choose
        triangle[2] = Vertex
            {high_lims.edge_point_in_3d(),
             txadapter(high_lims.edge_point_on_tile())};
        inserter.push(triangle);
    }
    if (!are_very_close(high_lims.edge_point_in_3d (),
                        high_lims.spine_point_in_3d()))
    {
        // can always refactor down to fewer lines if desired
        VertexTriangle triangle;
        triangle[0] = Vertex
            {high_lims.edge_point_in_3d(),
             txadapter(low_lims.edge_point_on_tile())};
        triangle[1] = Vertex
            {high_lims.spine_point_in_3d(),
             txadapter(low_lims.spine_point_on_tile())};
        // it does matter here, need to build a whole quad
        triangle[2] = Vertex
            {low_lims.spine_point_in_3d(),
             txadapter(low_lims.spine_point_on_tile())};
        inserter.push(triangle);
    }
}

void NorthSouthTwistTileGroup::load
    (const Rectangle & rectangle, const TileSetXmlGrid & xmlgrid,
     Platform & platform)
{
    auto geo_grid = make_twisty_geometry_for
        (size_of(rectangle), TwistDirection::left, TwistPathDirection::north_south,
         CapTexturingAdapter{top_left_of(rectangle), size_of(rectangle)},
         k_breaks_per_segment);
}

void NorthSouthTwistTileGroup::operator ()
    (const Vector2I & position_in_group, const Vector2I & tile_offset,
     EntityAndTrianglesAdder &, Platform &) const
{

}
