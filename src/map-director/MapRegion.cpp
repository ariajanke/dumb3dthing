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
#if 0
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
#endif
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

TiledMapRegion::TiledMapRegion
    (ProducableTileViewGrid && producables_view_grid,
     ScaleComputation && scale_computation):
    m_producables_view_grid(std::move(producables_view_grid)),
    m_scale(std::move(scale_computation)) {}

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

/* private */ void TiledMapRegion::process_load_request_
    (ProducableTileViewGrid::SubGrid producables,
     const RegionLoadRequest & request,
     const RegionPositionFraming & region_position_framing,
     RegionLoadCollectorBase & collector)
{
    auto collect_load_jobs = [&producables, &collector]
        (const RectangleI & subregion_bounds,
         const ScaleComputation & scale,
         const Vector2I & on_field_position)
    {
        collector.collect_load_job
            (SubRegionPositionFraming{scale, on_field_position},
             producables.make_sub_grid(cul::top_left_of(subregion_bounds), subregion_bounds.width, subregion_bounds.height));
    };
    region_position_framing.
        for_each_overlap(m_scale, request, producables.size2(),
                         std::move(collect_load_jobs));
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
