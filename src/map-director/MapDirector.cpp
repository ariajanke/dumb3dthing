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
