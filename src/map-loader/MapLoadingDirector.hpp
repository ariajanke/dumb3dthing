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

#pragma once

#include "../Defs.hpp"
#include "../Components.hpp"
#include "TiledMapLoader.hpp"

#include "MapRegionTracker.hpp"

namespace point_and_plane {
    class Driver;
} // end of point_and_plane namespace

/** @brief The MapLoadingDirector is responsible for loading map segments.
 *
 *  Map segments are loaded depending on the player's state.
 */
class MapLoadingDirector final {
public:
    using PpDriver = point_and_plane::Driver;

    MapLoadingDirector(PpDriver * ppdriver, Size2I chunk_size):
        m_ppdriver(ppdriver),
        m_chunk_size(chunk_size)
    {}

    SharedPtr<BackgroundTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform, const Entity & player_physics);

    void on_every_frame(TaskCallbacks & callbacks, const Entity & physics_ent);

private:
    static Vector2I to_segment_location
        (const Vector & location, const Size2I & segment_size);

    void check_for_other_map_segments
        (TaskCallbacks & callbacks, const Entity & physics_ent);

    // there's only one per game and it never changes
    PpDriver * m_ppdriver = nullptr;
    Size2I m_chunk_size;
    std::vector<TiledMapLoader> m_active_loaders;
    MapRegionTracker m_region_tracker;
};

/// all things the player needs to do everyframe
///
/// stuffing it in here, until there's a proper living place for this class
class PlayerUpdateTask final : public EveryFrameTask {
public:
    PlayerUpdateTask
        (MapLoadingDirector && map_director, const EntityRef & physics_ent):
        m_map_director(std::move(map_director)),
        m_physics_ent(physics_ent)
    {}

    SharedPtr<BackgroundTask> load_initial_map(const char * initial_map, Platform & platform);

    void on_every_frame(Callbacks & callbacks, Real) final;

private:
    static void check_fall_below(Entity & ent);

    MapLoadingDirector m_map_director;
    // | extremely important that the task is *not* owning
    // v the reason entity refs exists
    EntityRef m_physics_ent;
};
