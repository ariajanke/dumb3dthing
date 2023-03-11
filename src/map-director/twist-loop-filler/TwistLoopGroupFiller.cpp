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

#include "../TileFactory.hpp"
#include "../TileSetPropertiesGrid.hpp"
#include "../twist-loop-filler.hpp"

#include "../../Components.hpp"
#include "../../TriangleSegment.hpp"
#include "../../platform.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

constexpr const auto k_twisty_origin = TwistyStripRadii::k_twisty_origin;

namespace {

using namespace cul::exceptions_abbr;
using cul::size_of, cul::top_left_of;

} // end of <anonymous> namespace
#if 0
TwistyTileTValueLimits::TwistyTileTValueLimits
    (const Size2I & twisty_size, const Vector2I & tile_pos)
{
    throw BadBranchException{__LINE__, __FILE__};
    // need x positions for t = pos.x / width, (pos.x + 1) / width
    // if there's a cross, then I need to find that intersection

    //bool raise_lower_t_value = ;
    //bool high_radii_regular = high_radii.edge_offset() > high_radii.spine_offset();
#   if 0
    bool low_radii_regular  = low_radii .edge_offset() > low_radii .spine_offset();
    bool high_radii_regular = high_radii.edge_offset() > high_radii.spine_offset();
    if (low_radii_regular && high_radii_regular) {
        m_low_t_limit = range.low_t();
        m_high_t_limit = range.high_t();
    } else if (!low_radii_regular && !high_radii_regular) {
        // just shove both points into one corner and call it good
        m_low_t_limit = m_high_t_limit = range.low_t();
    } else {
        // gotta find that t s.t. is the spine side's x
        auto sol = [] (const Size2I & twisty_size, const Vector2I & tile_pos) {
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
#   endif
}
#endif
Optional<TwistyTileTValueLimits> TwistyTileTValueLimits::find
    (const Size2I & twisty_size, const Vector2I & tile_pos)
{
    using RadiiLims = TwistyStripRadii;
    TwistyTileTValueRange range{twisty_size, tile_pos};
    auto low_radii  = RadiiLims::find(twisty_size.width, tile_pos.x, range.low_t ());
    auto high_radii = RadiiLims::find(twisty_size.width, tile_pos.x, range.high_t());
    if (!low_radii && !high_radii) {
        return {};
    }
    // it's possible for there to be two intersections
    // it maybe good enough to just use the nearest
    auto intersecting_t_value_ = [twisty_size, &range] (int strip_x)
        { return intersecting_t_value(twisty_size, strip_x, range); };

    auto closest_alternate = [&intersecting_t_value_, tile_pos] {
        auto intersect_low  = intersecting_t_value_(tile_pos.x    );
        auto intersect_high = intersecting_t_value_(tile_pos.x + 1);
        return [=] (Real t) {
            if (intersect_low && intersect_high) {
                if (  magnitude(*intersect_low - t)
                    < magnitude(*intersect_high - t))
                { return *intersect_low; }
                return *intersect_high;
            }
            if (intersect_low ) return *intersect_low;
            if (intersect_high) return *intersect_high;
            throw BadBranchException{__LINE__, __FILE__};
        };
    } ();

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
#if 0
TwistyTileRadiiLimits::TwistyTileRadiiLimits
    (const Size2I & twisty_size, int tile_pos_x, Real t_value)
{
#   if 0
    // the needed radius to reach a point can be undefined if the twisty is
    // going straight up/down
    // spine point should come first, then edge point...

    // I have both points projected onto the plane of the twisty
    // now I try and limit them inside the bound's of that tile
    auto [low_edge_x, high_edge_x, low_dir] =
        low_high_x_edges_and_low_dir(twisty_size, t_value);

    // restrict x-ways
    auto [spine_side_x, edge_side_x] =
        spine_and_edge_side_x(twisty_size.width, tile_pos_x);

    // edge may slide into tile bounds, or use the tile bounds
    edge_side_x = std::max(low_edge_x, std::min(high_edge_x, edge_side_x));

    auto tan_theta = std::tan(t_value*2*k_pi);
    m_edge  = //edge_side_x *tan_theta;
    m_spine = //spine_side_x*tan_theta;
#   else
    auto max_x = std::tan(t_value*2*k_pi);
    throw BadBranchException{__LINE__, __FILE__};
#   endif
}

/* static */ Tuple<Real, Real, Real>
    TwistyTileRadiiLimits::low_high_x_edges_and_low_dir
    (const Size2I & twisty_size, Real t_value)
{
    auto spine_pt      = to_twisty_spine(Size2{twisty_size}, t_value);
    auto twisty_rad    = Real(twisty_size.width) / 2;
    auto ex_edge_pt_a  = spine_pt - to_twisty_offset(twisty_rad, t_value);
    auto ex_edge_pt_b  = spine_pt + to_twisty_offset(twisty_rad, t_value);
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
    auto tile_low_x  = k_twisty_origin.x + tile_pos_x;
    auto tile_high_x = tile_low_x + 1;
    auto twisty_rad  = twisty_width / 2;
    // which side of the spine are we on?
    if (  magnitude(tile_low_x  - twisty_rad)
        > magnitude(tile_high_x - twisty_rad))
    {
        return make_tuple(tile_high_x, tile_low_x);
    }
    return make_tuple(tile_low_x, tile_high_x);
}
#endif
#if 0
TwistyTilePointLimits::TwistyTilePointLimits
    (const Size2I & twisty_size, const Vector2I & tile_pos, Real t_value)
{
    TwistyTileRadiiLimits radii_lims{twisty_size, tile_pos.x, t_value};
    auto dir = to_twisty_offset(1, t_value);
    m_spine  = dir*(*radii_lims.spine_offset());
    m_edge   = dir*(*radii_lims.edge_offset ());
}
#endif
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
#       if 0
        TwistyTileTValueLimits lims{twisty_size, Vector2I{x, y}};
        tbreaks.push_back(lims.low_t_limit ());
        tbreaks.push_back(lims.high_t_limit());
#       endif
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
#       if 0
        TwistyTilePointLimits low_lims {twisty_size, inserter.position(), *last};
        TwistyTilePointLimits high_lims{twisty_size, inserter.position(), t};
        insert_twisty_geometry_into(inserter, txadapter, low_lims, high_lims);
#       else
        TwistyTileEdgePoints low_points {twisty_size, inserter.position().x, *last};
        TwistyTileEdgePoints high_points{twisty_size, inserter.position().x, t};
        insert_twisty_geometry_into(inserter, txadapter, low_points, high_points);
#       endif
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
#if 0
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
#endif
/* private */ void NorthSouthTwistTileGroup::load_
    (const RectangleI & rectangle, const TileSetXmlGrid &,
     Platform & platform)
{
    auto geo_grid = make_twisty_geometry_for
        (size_of(rectangle), TwistDirection::left, TwistPathDirection::north_south,
         CapTexturingAdapter{Vector2{top_left_of(rectangle)}, Size2{size_of(rectangle)}},
         k_breaks_per_segment);
    std::vector<Vertex> verticies;
    std::vector<unsigned> elements;
    ViewGridInserter<TriangleSegment> triangle_inserter{geo_grid.size2()};
    Grid<SharedPtr<const RenderModel>> render_models;
    render_models.set_size(geo_grid.size2(), nullptr);
    for (Vector2I r; r != geo_grid.end_position(); r = geo_grid.next(r)) {
        auto mod = platform.make_render_model();
        for (auto & triangle : geo_grid(r)) {
            triangle_inserter.push
                (TriangleSegment{triangle[0].position,
                                 triangle[1].position,
                                 triangle[2].position});
            for (auto & vtx : triangle) {
                verticies.push_back(vtx);
                elements.push_back(elements.size());
            }
        }
        triangle_inserter.advance();
        mod->load(verticies, elements);
        verticies.clear();
        elements.clear();
        render_models(r) = mod;
    }
    m_collision_triangles = ViewGrid<TriangleSegment>{std::move(triangle_inserter)};
    m_group_models = std::move(render_models);
}

void NorthSouthTwistTileGroup::operator ()
    (const Vector2I & position_in_group, const Vector2I & tile_offset,
     EntityAndTrianglesAdder & adder, Platform & platform) const
{
    // I forgot the formula
    auto v3_offset = TileFactory::grid_position_to_v3(tile_offset)
        + k_twisty_origin;
    for (auto & triangle : m_collision_triangles(position_in_group)) {
        adder.add_triangle(triangle.move(v3_offset));
    }
    auto entity = platform.make_renderable_entity();
    entity.add<SharedPtr<const RenderModel>, Translation, Visible>()
        = make_tuple
        (m_group_models(position_in_group), Translation{v3_offset}, Visible{});
}

class RectanglarGroupOfPred {
public:
    virtual ~RectanglarGroupOfPred() {}

    virtual bool operator () (const Vector2I &) const = 0;
};

RectangleI get_rectangular_group_of
    (const Vector2I & start, const RectanglarGroupOfPred &);

template <typename Func>
using EnableRectanglarGroupOfPredType =
    std::enable_if_t<!std::is_base_of_v<RectanglarGroupOfPred, Func>, std::monostate>;

template <typename Func>
RectangleI get_rectangular_group_of
    (const Vector2I & start, Func &&, EnableRectanglarGroupOfPredType<Func> = {});

template <typename Func>
RectangleI get_rectangular_group_of
    (const Vector2I & start, Func && f, EnableRectanglarGroupOfPredType<Func>)
{
    class Impl final : public RectanglarGroupOfPred {
    public:
        explicit Impl(Func && f_): m_f(std::move(f_)) {}

        bool operator () (const Vector2I & r) const final
            { return m_f(r); }

    private:
        Func m_f;
    };
    return get_rectangular_group_of(start, Impl{std::move(f)});
}

RectangleI get_rectangular_group_of
    (const Vector2I & start, const RectanglarGroupOfPred & is_in_group)
{
    auto x_end = start.x;
    for (; is_in_group(Vector2I{x_end, start.y}); ++x_end) {}
    if (x_end == start.x) {
        // this ain't right
        return RectangleI{};
    }

    for (int height = 0; true; ++height) {
        bool row_good = true;
        for (int x = start.x; x != x_end && row_good; ++x) {
            row_good = row_good && is_in_group(Vector2I{x, start.y + height});
        }

        if (!row_good) {
            return RectangleI{start, Size2I{x_end - start.x, height}};
        }
    }
}

void TwistLoopGroupFiller::load
    (const TileSetXmlGrid & xml_grid, Platform & platform)
{
    Grid<bool> checked;
    checked.set_size(xml_grid.size2(), false);
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        if (checked(r)) continue;
        if (xml_grid(r).type() != twist_loop_filler_names::k_ns_twist_loop) {
            checked(r) = true;
            continue;
        }
        auto origin_type = xml_grid(r).type();
        auto rect_group = get_rectangular_group_of(r,
            [&xml_grid, &checked] (const Vector2I & r)
        {
            if (!xml_grid.has_position(r) || checked(r)) return false;
            return xml_grid(r).type() == twist_loop_filler_names::k_ns_twist_loop;
        });
        // make a group
        auto twist_filler = make_shared<NorthSouthTwistTileGroup>();
        twist_filler->load(rect_group, xml_grid, platform);
        // std::fill doesn't work with containers
        auto checked_subgrid = cul::make_sub_grid(checked, rect_group);
        for (Vector2I r; r != checked_subgrid.end_position(); r = checked_subgrid.next(r)) {
            checked_subgrid(r) = true;
        }
        for (auto & ptr : cul::make_sub_grid(m_tile_groups, rect_group)) {
            ptr = twist_filler;
        }
    }
}

UnfinishedTileGroupGrid TwistLoopGroupFiller::operator ()
    (const std::vector<TileLocation> & positions,
     UnfinishedTileGroupGrid && unfinished_group_grid) const
{
    UnfinishedProducableGroup<TwistLoopTile> producable_group;
    for (auto & pos : positions) {
        auto tile_group = m_tile_groups(pos.on_tileset);
        if (!tile_group) continue; // <- a baffling situation indeed!
        producable_group.at_location(pos.on_map).make_producable
            (pos.on_map, pos.on_tileset - tile_group->group_start(), tile_group);
    }
    unfinished_group_grid.add_group(std::move(producable_group));
    return std::move(unfinished_group_grid);
}
