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

#include "RegionPositionFraming.hpp"

#include "RegionEdgeConnectionsContainer.hpp"
#include "RegionLoadRequest.hpp"

namespace {

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

} // end of <anonymous> namespace

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
          //SubRegionPositionFraming{scale, on_field_position}
          scale, on_field_position);
    }}
}
