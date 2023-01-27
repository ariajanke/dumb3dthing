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

#include "../map-director.hpp"
#include "TiledMapLoader.hpp"
#include "MapRegionTracker.hpp"

/** @brief The MapDirector turns player physics things into map region
 *         loading/unloading.
 *
 *  Map segments are loaded depending on the player's state.
 */
class MapDirector final : public MapDirector_ {
public:
    using PpDriver = point_and_plane::Driver;

    MapDirector(PpDriver * ppdriver, Size2I chunk_size):
        m_ppdriver(ppdriver),
        m_region_size_in_tiles(chunk_size)
    {}

    SharedPtr<BackgroundTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform,
         const Entity & player_physics) final;

    void on_every_frame
        (TaskCallbacks & callbacks, const Entity & physics_ent) final;

private:
    static Vector2I to_segment_location
        (const Vector & location, const Size2I & segment_size);

    void check_for_other_map_segments
        (TaskCallbacks & callbacks, const Entity & physics_ent);

    // there's only one per game and it never changes
    PpDriver * m_ppdriver = nullptr;
    Size2I m_region_size_in_tiles;
    SharedPtr<MapRegionTracker> m_region_tracker;
};
