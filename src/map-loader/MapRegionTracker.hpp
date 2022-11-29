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

#include "MapRegion.hpp"

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    using Size2I = cul::Size2<int>;
    using NeighborRegions = MapRegionPreparer::NeighborRegions;

    MapRegionTracker() {}

    MapRegionTracker
        (UniquePtr<MapRegion> && root_region,
         const Size2I & region_size_in_tiles);

    void frame_refresh(TaskCallbacks & callbacks);

    void frame_hit(const Vector2I & global_region_location, TaskCallbacks & callbacks);

private:
    class TrackerLoadedRegion final : public LoadedMapRegion {
    public:
        bool keep_on_refresh = true;
    };

    using LoadedRegionMap = std::unordered_map
        <Vector2I, TrackerLoadedRegion, Vector2IHasher>;

    NeighborRegions get_neighbors_for(const Vector2I & r);

    LoadedMapRegion * location_to_pointer(const Vector2I & r);

    LoadedRegionMap m_loaded_regions;
    UniquePtr<MapRegion> m_root_region;
    Size2I m_region_size_in_tiles;
};
