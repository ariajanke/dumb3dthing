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

#include "Components.hpp"

namespace point_and_plane {
    class Driver;
} // end of point_and_plane namespace

class MapDirector_ {
public:
    using PpDriver = point_and_plane::Driver;

    ~MapDirector_() {}

    static SharedPtr<MapDirector_> make(PpDriver *, Size2I chunk_size);

    virtual SharedPtr<BackgroundTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform,
         const Entity & player_physics) = 0;

    virtual void on_every_frame
        (TaskCallbacks & callbacks, const Entity & physics_ent) = 0;
};
