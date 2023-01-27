/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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
#include "../PointAndPlaneDriver.hpp"

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

SharedPtr<BackgroundTask> MapDirector::begin_initial_map_loading
    (const char * initial_map, Platform & platform, const Entity & player_physics)
{
    TiledMapLoader map_loader
        {platform, initial_map, Vector2I{}};
    // presently: task will be lost without completing
    return BackgroundTask::make(
        [this, map_loader = std::move(map_loader),
         player_physics_ = player_physics]
        (TaskCallbacks &) mutable {
            auto grid = map_loader.update_progress();
            if (!grid) return BackgroundCompletion::in_progress;
            player_physics_.ensure<Velocity>();
            m_region_tracker = MapRegionTracker
                {make_unique<TiledMapRegion>
                    (std::move(*grid), m_chunk_size),
                 m_chunk_size};
            return BackgroundCompletion::finished;
        });
}

void MapDirector::on_every_frame
    (TaskCallbacks & callbacks, const Entity & physic_ent)
{
    m_active_loaders.erase
        (std::remove_if(m_active_loaders.begin(), m_active_loaders.end(),
                        [](const TiledMapLoader & loader) { return loader.is_expired(); }),
         m_active_loaders.end());

    check_for_other_map_segments(callbacks, physic_ent);
}

/* private static */ Vector2I MapDirector::to_segment_location
    (const Vector & location, const Size2I & segment_size)
{
    return Vector2I
        {int(std::floor( location.x / segment_size.width )),
         int(std::floor(-location.z / segment_size.height))};
}

/* private */ void MapDirector::check_for_other_map_segments
    (TaskCallbacks & callbacks, const Entity & physics_ent)
{
    // this may turn into its own class
    // there's just so much behavior potential here

    // good enough for now
    using namespace point_and_plane;
    auto & pstate = physics_ent.get<PpState>();
    auto delta = physics_ent.has<Velocity>() ? physics_ent.get<Velocity>()*0.5 : Vector{};
    for (auto pt : { location_of(pstate), location_of(pstate) + delta }) {
        auto target_region = to_segment_location(pt, m_chunk_size);
        m_region_tracker.frame_hit(target_region, callbacks);
        for (auto offset : k_plus_shape_neighbor_offsets) {
            m_region_tracker.frame_hit(offset + target_region, callbacks);
        }
    }
    m_region_tracker.frame_refresh(callbacks);
}
