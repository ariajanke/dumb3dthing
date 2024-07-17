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

#include "map-director.hpp"
// going down the directory tree is okay here, as this file acts like an
// attorney class, but in source file structure form
#include "map-director/MapDirector.hpp"

/* static */ SharedPtr<BackgroundTask> MapDirector_::begin_initial_map_loading
    (Entity player_physics,
     const char * initial_map,
     PlatformAssetsStrategy & platform,
     PpDriver & ppdriver)
{
    return MapDirector::begin_initial_map_loading
        (player_physics, initial_map, platform, ppdriver);
}
