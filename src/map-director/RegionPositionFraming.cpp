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
     const RegionLoadRequestBase & request)
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

TilePositionFraming::TilePositionFraming
    (const ScaleComputation & scale,
     const Vector2I & on_field_position,
     const Vector2I & inserter_position):
    m_scale(scale),
    m_on_field_region_position(on_field_position),
    m_on_field_tile_position(on_field_position + inserter_position) {}

TriangleSegment TilePositionFraming::transform(const TriangleSegment & triangle) const
    { return triangle_transformation()(triangle); }

ModelScale TilePositionFraming::model_scale() const
    { return triangle_transformation().model_scale(); }

ModelTranslation TilePositionFraming::model_translation() const
    { return triangle_transformation().model_translation(); }

/* private */ TriangleSegmentTransformation
    TilePositionFraming::triangle_transformation() const
    { return TriangleSegmentTransformation{m_scale, m_on_field_tile_position}; }

// ----------------------------------------------------------------------------

void SubRegionPositionFraming::set_containers_with
    (SharedPtr<ViewGridTriangle> && triangle_grid_ptr,
     std::vector<Entity> && entities,
     MapRegionContainer & container,
     RegionEdgeConnectionsAdder & edge_container_adder) const
{
    ScaledTriangleViewGrid scaled_view_grid{triangle_grid_ptr, m_scale};
    container.set_region
        (m_on_field_position, scaled_view_grid, std::move(entities));
    edge_container_adder.add(m_on_field_position, scaled_view_grid);
}

// ----------------------------------------------------------------------------

RegionPositionFraming::RegionPositionFraming
    (const ScaleComputation & tile_scale,
     const Vector2I & on_field_position):
    m_on_field_position(on_field_position),
    m_tile_scale(tile_scale) {}

bool RegionPositionFraming::operator ==
    (const RegionPositionFraming & rhs) const
{
    return m_on_field_position    == rhs.m_on_field_position &&
           m_tile_scale           == rhs.m_tile_scale;
}

SubRegionPositionFraming RegionPositionFraming::as_sub_region_framing() const
    { return SubRegionPositionFraming{m_tile_scale, m_on_field_position}; }

RegionPositionFraming RegionPositionFraming::move
    (const Vector2I & map_tile_position) const
{
    return RegionPositionFraming
        {m_tile_scale, m_on_field_position + m_tile_scale(map_tile_position)};
}

RegionPositionFraming RegionPositionFraming::with_scaling
    (const ScaleComputation & map_scale) const
{ return RegionPositionFraming{map_scale, m_on_field_position}; }

/* private */ void RegionPositionFraming::for_each_overlap_
    (const Size2I & region_size,
     const RegionLoadRequestBase & request,
     const OverlapFunc & f) const
{
    const auto step = region_load_step(region_size, request);
    const auto subgrid_size = cul::convert_to<Size2I>(step);
    for (Vector2I r; r.x < region_size.width ; r.x += step.x) {
    for (r.y = 0   ; r.y < region_size.height; r.y += step.y) {
        auto on_field_position = m_on_field_position + m_tile_scale(r);
        RectangleI on_field_rect{on_field_position, m_tile_scale(subgrid_size)};
        bool overlaps_this_subregion = request.
            overlaps_with(on_field_rect);
        if (!overlaps_this_subregion) continue;

        f(move(r), RectangleI{r, subgrid_size});
    }}
}
