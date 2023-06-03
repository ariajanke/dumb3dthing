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

#include "MapDirector.hpp"
#include "map-loader-task.hpp"
#include "RegionLoadRequest.hpp"

#include "../point-and-plane.hpp"

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace
#if 0
RegionLoadRequest::RegionLoadRequest
    (const Vector2 & triangle_a, const Vector2 & triangle_b,
     const Vector2 & triangle_c, Size2I max_region_size):
    m_triangle_bounds(bounds_for(triangle_a, triangle_b, triangle_c)),
    m_pt_a(triangle_a),
    m_pt_b(triangle_b),
    m_pt_c(triangle_c),
    m_max_size(max_region_size)
{}

/* static */ RegionLoadRequest RegionLoadRequest::find
    (const Vector & player_position, const Optional<Vector> & player_facing,
     const Vector & player_velocity, Size2I max_region_size)
{
    auto triangle = find_triangle(player_position, player_facing, player_velocity);
    return RegionLoadRequest
        {to_global_tile_position(triangle.point_a()),
         to_global_tile_position(triangle.point_b()),
         to_global_tile_position(triangle.point_c()),
         max_region_size};
}

/* static */ TriangleSegment RegionLoadRequest::find_triangle
    (const Vector & player_position, const Optional<Vector> & player_facing,
     const Vector & player_velocity)
{
    static constexpr Real k_max_speed   = 8;
    static constexpr Real k_low_offset  = 4;
    static constexpr Real k_high_offset = 1;
    static_assert(k_low_offset > k_high_offset);
    static constexpr Real k_out_point_offset_low  = k_low_offset + 6;
    static constexpr Real k_out_point_offset_high = k_low_offset + 10;

    // angles are controlled by a fixed area
    // the resulting triangle will be an isosceles
    static auto interpol = [] (Real t, Real low, Real high)
        { return t*high + (t - 1)*low; };

    Real normalized_speed =
        std::min(magnitude(player_velocity), k_max_speed) / k_max_speed;
    auto a = player_position -
             interpol(normalized_speed, k_low_offset, k_high_offset)*
             player_facing.value_or(Vector{1, 0, 0});
    auto out_point_offset =
        interpol(normalized_speed, k_out_point_offset_low, k_out_point_offset_high);
    auto to_out_point = normalize(player_position - a)*out_point_offset;
    auto out_point = a + to_out_point;
    auto out_point_offset_to_bc = k_triangle_area / out_point_offset;
    auto to_bc_dir = normalize(cross(k_plane_normal, to_out_point));
    auto b = out_point + out_point_offset_to_bc*to_bc_dir;
    auto c = out_point - out_point_offset_to_bc*to_bc_dir;
    return TriangleSegment{a, b, c};
}
#endif
// ----------------------------------------------------------------------------

SharedPtr<BackgroundTask> MapDirector::begin_initial_map_loading
    (const char * initial_map, Platform & platform, const Entity & player_physics)
{
    m_region_tracker = make_shared<MapRegionTracker>();

    return MapLoaderTask_::make
        (initial_map, platform, m_region_tracker, player_physics);
}

void MapDirector::on_every_frame
    (TaskCallbacks & callbacks, const Entity & physic_ent)
{ check_for_other_map_segments(callbacks, physic_ent); }

/* private static */ Vector2I MapDirector::to_region_location
    (const Vector & location, const Size2I & segment_size)
{
    return Vector2I
        {int(std::floor( location.x / segment_size.width )),
         int(std::floor(-location.z / segment_size.height))};
}

/* private */ void MapDirector::check_for_other_map_segments
    (TaskCallbacks & callbacks, const Entity & physics_ent)
{
    // should either be strongly not taken, or strongly taken
    if (!m_region_tracker->has_root_region()) return;
    // this may turn into its own class
    // there's just so much behavior potential here
#   if 0
    // good enough for now
    using namespace point_and_plane;
    auto & pstate = physics_ent.get<PpState>();
    auto delta = physics_ent.has<Velocity>() ? physics_ent.get<Velocity>()*0.5 : Vector{};
    for (auto pt : { location_of(pstate), location_of(pstate) + delta }) {
        auto target_region = to_region_location(pt, MapRegion::k_temp_region_size);
        m_region_tracker->frame_hit(target_region, callbacks);
        for (auto offset : k_plus_shape_neighbor_offsets) {
            m_region_tracker->frame_hit(offset + target_region, callbacks);
        }
    }
    m_region_tracker->frame_refresh(callbacks);
#   endif
    auto facing = [&physics_ent] () -> Optional<Vector> {
        auto & camera = physics_ent.get<Camera>();
        if (!are_very_close(camera.target, camera.position))
            return {};
        return normalize(camera.target - camera.position);
    } ();
    auto request = RegionLoadRequest::find
        (point_and_plane::location_of(physics_ent.get<PpState>()),
         facing,
         physics_ent.get<Velocity>().value,
         MapRegion::k_temp_region_size);
    m_region_tracker->frame_hit(request, callbacks);
}

Vector2 to_global_tile_position(const Vector & r)
    { return Vector2{ r.x, -r.z } + Vector2{ 0.5, 0.5 }; }
