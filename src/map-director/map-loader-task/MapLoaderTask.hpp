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

#pragma once

#include "../TiledMapLoader.hpp"
#include "../MapRegionTracker.hpp"

class MapLoaderTask final : public BackgroundTask {
public:
    MapLoaderTask
        (TiledMapLoader && map_loader,
         const SharedPtr<MapRegionTracker> & target_region_instance,
         const Entity & player_physics, const Size2I & region_size_in_tiles);

    BackgroundCompletion operator () (Callbacks &) final;

private:
    static const SharedPtr<MapRegionTracker> &
        verify_region_tracker_presence
        (const char * caller, const SharedPtr<MapRegionTracker> &);

    SharedPtr<MapRegionTracker> m_region_tracker;
    TiledMapLoader m_map_loader;
    EntityRef m_player_physics;
    Size2I m_region_size_in_tiles;
};
