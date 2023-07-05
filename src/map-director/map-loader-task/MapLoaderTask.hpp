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

#include "TiledMapLoader.hpp"

#include "../MapRegionTracker.hpp"

#include "../../Components.hpp"

class MapLoaderTask final : public BackgroundTask {
public:
    MapLoaderTask
#        ifndef MACRO_NEW_MAP_LOADER_STATES
        (TiledMapLoader && map_loader,
#        else
        (tiled_map_loading::MapLoadStateMachine && map_loader,
#        endif
         const SharedPtr<MapRegionTracker> & target_region_instance,
         const Entity & player_physics);

    BackgroundCompletion operator () (Callbacks &) final;

private:
    static const SharedPtr<MapRegionTracker> &
        verify_region_tracker_presence
        (const char * caller, const SharedPtr<MapRegionTracker> &);

    SharedPtr<MapRegionTracker> m_region_tracker;
#   ifndef MACRO_NEW_MAP_LOADER_STATES
    TiledMapLoader m_map_loader;
#   else
    tiled_map_loading::MapLoadStateMachine m_map_loader;
#   endif
    EntityRef m_player_physics;
};
