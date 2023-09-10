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

#include "MapRegion.hpp"
#include "MapRegionTracker.hpp"
#include "ParseHelpers.hpp"

#include "../TriangleSegment.hpp"

#include <iostream>

#include <cstring>

namespace {

using namespace cul::exceptions_abbr;

Vector2I region_load_step
    (const ProducableTileViewGrid &, const RegionLoadRequest &);

template <typename Func>
void for_each_step_region
    (const ProducableTileViewGrid &,
     const RegionLoadRequest &,
     Func &&);

Vector2I region_load_step
    (const Size2I & region_size,
     const RegionLoadRequest & request)
{
    auto step_of_ = [] (int length, int max) {
        // I want as even splits as possible
        if (length < max) return length;
        return length / (length / max);
    };
    return Vector2I
        {step_of_(region_size.width , request.max_region_size().width ),
         step_of_(region_size.height, request.max_region_size().height)};
}

constexpr const auto k_comma_splitter = [](char c) { return c == ','; };
constexpr const auto k_whitespace_trimmer =
    make_trim_whitespace<const char *>();

} // end of <anonymous> namespace

/* static */ Optional<ScaleComputation>
    ScaleComputation::parse(const char * string)
{
    std::array<Real, 3> args;
    auto read_pos = args.begin();
    auto data_substrings = split_range
        (string, string + ::strlen(string), k_comma_splitter,
         k_whitespace_trimmer);
    for (auto data_substring : data_substrings) {
        // too many arguments
        if (read_pos == args.end()) return {};
        auto read_number = cul::string_to_number
            (data_substring.begin(), data_substring.end(), *read_pos++);
        if (!read_number) return {};
    }
    switch (read_pos - args.begin()) {
    case 1: return ScaleComputation{args[0], args[0], args[0]};
    case 3: return ScaleComputation{args[0], args[1], args[2]};
    // 0 or 2 arguments
    default: return {};
    }
}

ScaleComputation::ScaleComputation
    (Real eastwest_factor,
     Real updown_factor,
     Real northsouth_factor):
    m_factor(eastwest_factor, updown_factor, northsouth_factor) {}

TriangleSegment ScaleComputation::
    operator () (const TriangleSegment & triangle) const
{
    // this is also LoD, but I don't wanna load up triangle segment too much
    return TriangleSegment
        {scale(triangle.point_a()),
         scale(triangle.point_b()),
         scale(triangle.point_c())};
}

ModelScale ScaleComputation::to_model_scale() const
    { return ModelScale{m_factor}; }

Vector ScaleComputation::scale(const Vector & r) const
    { return Vector{r.x*m_factor.x, r.y*m_factor.y, r.z*m_factor.z}; }

// ----------------------------------------------------------------------------

void SubRegionPositionFraming::set_containers_with
    (SharedPtr<ViewGridTriangle> && triangle_grid_ptr,
     std::vector<Entity> && entities,
     MapRegionContainer & container,
     RegionEdgeConnectionsAdder & edge_container_adder) const
{
    container.set_region
        (m_on_field_position, triangle_grid_ptr, std::move(entities));
    edge_container_adder.add(m_on_field_position, triangle_grid_ptr);
}

// ----------------------------------------------------------------------------

TiledMapRegion::TiledMapRegion
    (ProducableTileViewGrid && producables_view_grid,
     ScaleComputation && scale_computation):
    m_producables_view_grid(std::move(producables_view_grid)),
    m_scale(std::move(scale_computation)) {}

/* private */ void RegionPositionFraming::for_each_overlap_
    (const ScaleComputation & scale,
     const RegionLoadRequest & request,
     const Size2I & region_size,
     const OverlapFunc & f) const
{
    const auto step = region_load_step(region_size, request);
    const auto subgrid_size = cul::convert_to<Size2I>(step);
    for (Vector2I r; r.x < region_size.width ; r.x += step.x) {
    for (r.y = 0   ; r.y < region_size.height; r.y += step.y) {
        auto on_field_position = m_spawn_offset + r;
        bool overlaps_this_subregion = request.
            overlaps_with(scale(RectangleI{on_field_position, subgrid_size}));
        if (!overlaps_this_subregion) continue;

        // m_spawn_offset - m_sub_region_position??

        f(RectangleI{r, subgrid_size},
          SubRegionPositionFraming{scale, on_field_position});
    }}
}


// takes the space of exactly one tile in the composite map
class TiledMapSubRegion final {
public:
    TiledMapSubRegion
        (const RectangleI &,
         const SharedPtr<MapRegion> &);

    void process_load_request
        (const RegionLoadRequest & request,
         const Vector2I & offset,
         RegionLoadCollectorBase & collector)
    {
        m_parent_region->process_limited_load_request
            (request, offset, m_sub_region_bounds, collector);
    }

    void process_load_request
        (const RegionLoadRequest & request,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase & collector)
    {
        // how does scaling change as you go up the stack?
        framing.for_each_overlap
            (ScaleComputation{}, request,
             Size2I{10, 10},
             [](const RectangleI & rect, const SubRegionPositionFraming & framing)
        {

            //framing.on_field_position();
            // framing.on_producable_run()
        });
    }

private:
    RectangleI m_sub_region_bounds;
    SharedPtr<MapRegion> m_parent_region;
};

void TiledMapRegion::process_load_request
    (const RegionLoadRequest & request,
     const RegionPositionFraming & framing,
     RegionLoadCollectorBase & collector)
{
    process_load_request_
        (m_producables_view_grid.make_subgrid(),
         request, framing, collector);
}

void TiledMapRegion::process_limited_load_request
    (const RegionLoadRequest & request,
     const RegionPositionFraming & framing,
     const RectangleI & grid_scope,
     RegionLoadCollectorBase & collector)
{
    process_load_request_
        (m_producables_view_grid.make_subgrid(grid_scope),
         request, framing, collector);
}

/* private */ void TiledMapRegion::collect_producables
    (const Vector2I & position_in_region,
     const Vector2I & offset,
     const Size2I & subgrid_size,
     RegionLoadCollectorBase & collector)
{
    auto on_field_position = offset + position_in_region;
    auto subgrid = m_producables_view_grid.
        make_subgrid(RectangleI{position_in_region, subgrid_size});
}

/* private */ void TiledMapRegion::process_load_request_
    (ProducableTileViewGrid::SubGrid producables,
     const RegionLoadRequest & request,
     const RegionPositionFraming & region_position_framing,
     RegionLoadCollectorBase & collector)
{
    auto collect_load_jobs = [&producables, &collector]
        (const RectangleI & subregion_bounds,
         const SubRegionPositionFraming & region_framing)
    {
        collector.collect_load_job
            (region_framing,
             producables.make_sub_grid(cul::top_left_of(subregion_bounds), subregion_bounds.width, subregion_bounds.height));
    };
    region_position_framing.
        for_each_overlap(m_scale, request, producables.size2(),
                         std::move(collect_load_jobs));
}

void CompositeMapRegion::process_load_request
    (const RegionLoadRequest & request,
     const RegionPositionFraming &,
     RegionLoadCollectorBase & collector)
{
    // will need to be able to spawn tasks
    // rescaling of request?
    Vector2I step;
    Size2I subgrid_size;
    int width = 0, height = 0;
    // it's like I want an intersection of a
    // "step sub region" and "composite map sub region"
    for (Vector2I r; r.x < width ; r.x += step.x) {
    for (r.y = 0   ; r.y < height; r.y += step.y) {

        ;
    }}
}

void CompositeMapRegion::process_limited_load_request
    (const RegionLoadRequest &,
     const RegionPositionFraming &,
     const RectangleI & grid_scope,
     RegionLoadCollectorBase &)
{}

void CompositeMapRegion::collect_load_tasks
    (const Vector2I & position_in_region,
     const Vector2I & offset,
     const Size2I & subgrid_size,
     RegionLoadCollectorBase & collector)
{
    // subgrid of type load task...
    // ... null hit -> do nothing
    // ... hit -> load maps AND load producables like normal

}

// ----------------------------------------------------------------------------

namespace {

Vector2I region_load_step
    (const ProducableTileViewGrid &, const RegionLoadRequest &);

template <typename Func>
void for_each_step_region
    (const ProducableTileViewGrid & producables_view_grid,
     const RegionLoadRequest & request,
     Func && f)
{
    const auto step = region_load_step(producables_view_grid, request);
    const auto subgrid_size = cul::convert_to<Size2I>(step);
    for (Vector2I r; r.x < producables_view_grid.width (); r.x += step.x) {
    for (r.y = 0   ; r.y < producables_view_grid.height(); r.y += step.y) {
        (void)f(r, subgrid_size);
    }}
}

Vector2I region_load_step
    (const ProducableTileViewGrid & view_grid,
     const RegionLoadRequest & request)
{
    auto step_of_ = [] (int length, int max) {
        // I want as even splits as possible
        if (length < max) return length;
        return length / (length / max);
    };
    return Vector2I
        {step_of_(view_grid.width (), request.max_region_size().width ),
         step_of_(view_grid.height(), request.max_region_size().height)};
}

} // end of <anonymous> namespace
