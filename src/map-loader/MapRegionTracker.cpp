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

#include "MapRegionTracker.hpp"
#if 0
namespace {

using NeighborRegions = MapRegionTracker::NeighborRegions;

} // end of <anonymous> namespace
#endif
MapRegionTracker::MapRegionTracker
    (UniquePtr<MapRegion> && root_region,
     const Size2I & region_size_in_tiles):
    m_root_region(std::move(root_region)),
    m_region_size_in_tiles(region_size_in_tiles) {}

void MapRegionTracker::frame_refresh(TaskCallbacks & callbacks) {
#   if 0
    for (auto itr = m_loaded_regions.begin(); itr != m_loaded_regions.end(); ) {
        if (itr->second.keep_on_refresh) {
            itr->second.keep_on_refresh = false;
            ++itr;
        } else {
            // teardown task handles removal of entities and physical triangles
            callbacks.add(itr->second.teardown);
            itr = m_loaded_regions.erase(itr);
        }
    }
#   endif
    m_loaded_regions.frame_refresh(callbacks);
}

void MapRegionTracker::frame_hit
    (const Vector2I & global_region_location, TaskCallbacks & callbacks)
{
    using RegionCompleter = MapRegionPreparer::RegionCompleter;

    if (!m_root_region) return;
    if (m_loaded_regions.find_and_keep(global_region_location))
        return;

    m_loaded_regions.ensure_region_available(global_region_location);
    Vector2I region_tile_offset
        {global_region_location.x*m_region_size_in_tiles.width,
         global_region_location.y*m_region_size_in_tiles.height};
    auto region_preparer = make_shared<MapRegionPreparer>(region_tile_offset);
    region_preparer->set_completer
        (RegionCompleter{global_region_location, m_loaded_regions});
    m_root_region->request_region_load(global_region_location, region_preparer, callbacks);

#   if 0
    if (auto * loaded_region = m_loaded_regions.find(global_region_location)) {
        loaded_region->keep_on_refresh = true;
    } else {

        m_loaded_regions[global_region_location];
        Vector2I region_tile_offset
            {global_region_location.x*m_region_size_in_tiles.width,
             global_region_location.y*m_region_size_in_tiles.height};
        auto region_preparer = make_shared<MapRegionPreparer>(region_tile_offset);
        region_preparer->set_completer
            (RegionCompleter{global_region_location, m_loaded_regions});
        m_root_region->request_region_load(global_region_location, region_preparer, callbacks);
    }
#   endif
#   if 0
    auto itr = m_loaded_regions.find(global_region_location);
    if (itr == m_loaded_regions.end()) {
        // I've a task that creates the triangles, entities, and links the
        // edges. This task is for a fully loaded region.

        // In order to do this I need a target map region.

        auto & loaded_region = m_loaded_regions[global_region_location];
        Vector2I region_tile_offset
            {global_region_location.x*m_region_size_in_tiles.width,
             global_region_location.y*m_region_size_in_tiles.height};
#       if 0
        auto region_preparer = make_shared<MapRegionPreparer>
            (loaded_region, region_tile_offset,
             get_neighbors_for(global_region_location));
        // A call into region, may do two things:
        // 1.) generate a task itself (which can then do 2)
        // 2.) immediately launch the aforementioned creation task
        m_root_region->request_region_load(global_region_location, region_preparer, callbacks);
#       endif
    } else {
        // move to top of LRU or whatever
        itr->second.keep_on_refresh = true;
    }
#   endif
}
#if 0
/* private */ NeighborRegions MapRegionTracker::get_neighbors_for(const Vector2I & r) {
    // no easy way to do this in C++ :c
    static constexpr const auto k_neighbors = k_plus_shape_neighbor_offsets;
    return NeighborRegions{
        location_to_pointer(r + k_neighbors[0]),
        location_to_pointer(r + k_neighbors[1]),
        location_to_pointer(r + k_neighbors[2]),
        location_to_pointer(r + k_neighbors[3])
    };
}

/* private */ LoadedMapRegion * MapRegionTracker::location_to_pointer(const Vector2I & r) {
    auto itr = m_loaded_regions.find(r);
    if (itr == m_loaded_regions.end()) return nullptr;
    return &itr->second;
}
#endif
