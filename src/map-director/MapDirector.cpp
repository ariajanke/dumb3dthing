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
    (const char * initial_map, Platform & platform)
{
    m_region_tracker = make_shared<MapRegionTracker>();
    return MapLoaderTask_::make(initial_map, platform, m_region_tracker);
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
    Entity{physics_ent}.ensure<Velocity>();
    // this may turn into its own class
    // there's just so much behavior potential here

    auto facing = [&physics_ent] () -> Optional<Vector> {
        auto & camera = physics_ent.get<Camera>();
        if (!are_very_close(camera.target, camera.position))
            return {};
        return normalize(camera.target - camera.position);
    } ();
    auto player_position = point_and_plane::location_of(physics_ent.get<PpState>());
    auto player_velocity = physics_ent.get<Velocity>().value;
    auto request = RegionLoadRequest::find
        (player_position, facing, player_velocity);
    m_region_tracker->process_load_requests(request, callbacks);
}
