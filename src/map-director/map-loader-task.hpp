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

#include "MapObjectCollection.hpp"

#include "../Tasks.hpp"

class MapRegion;

class MapLoaderTask_ : public BackgroundTask {
public:
    struct Result final {
        UniquePtr<MapRegion> map_region;
        MapObjectCollection map_objects;
        MapObjectFraming object_framing;
    };

    static SharedPtr<MapLoaderTask_> make
        (const char * initial_map, Platform & platform);

    /// @throws if the task has not finished
    virtual Result retrieve() = 0;
};
