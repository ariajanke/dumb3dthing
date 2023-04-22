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

#include "MapRegionTracker.hpp"

#include "../Components.hpp"

class MapLoaderTask_ {
public:
    static SharedPtr<BackgroundTask> make
        (const char * initial_map, Platform & platform,
         const SharedPtr<MapRegionTracker> & target_region_instance,
         const Entity & player_physics, const Size2I & region_size_in_tiles);
};
