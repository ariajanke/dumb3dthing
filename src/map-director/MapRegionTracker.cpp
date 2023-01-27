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

void MapRegionContainer::on_complete
    (const Vector2I & region_position,
     InterTriangleLinkContainer && link_container,
     SharedPtr<TeardownTask> && teardown_task)
{
    static constexpr const auto k_neighbors = k_plus_shape_neighbor_offsets;

    auto * loaded_region = find(region_position);
    if (!loaded_region) return;
    loaded_region->link_edge_container = std::move(link_container);
    loaded_region->teardown = std::move(teardown_task);

    for (auto n : k_neighbors) {
        auto neighbor = find(n + region_position);
        if (!neighbor) continue;
        loaded_region->link_edge_container.glue_to
            (neighbor->link_edge_container);
    }
}

void MapRegionContainer::ensure_region_available(const Vector2I & r)
    { (void)m_loaded_regions[r]; }

bool MapRegionContainer::find_and_keep(const Vector2I & r) {
    if (auto * region = find(r)) {
        return region->keep_on_refresh = true;
    }
    return false;
}

void MapRegionContainer::frame_refresh(TaskCallbacks & callbacks) {
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
}

/* private */ MapRegionContainer::LoadedMapRegion *
    MapRegionContainer::find(const Vector2I & r)
{
    auto itr = m_loaded_regions.find(r);
    if (itr == m_loaded_regions.end())
        return nullptr;
    return &itr->second;
}

// ----------------------------------------------------------------------------

MapRegionTracker::MapRegionTracker
    (UniquePtr<MapRegion> && root_region,
     const Size2I & region_size_in_tiles):
    m_root_region(std::move(root_region)),
    m_region_size_in_tiles(region_size_in_tiles) {}

void MapRegionTracker::frame_refresh(TaskCallbacks & callbacks) {
    m_loaded_regions.frame_refresh(callbacks);
}

void MapRegionTracker::frame_hit
    (const Vector2I & global_region_location, TaskCallbacks & callbacks)
{
    if (!m_root_region) return;
    if (m_loaded_regions.find_and_keep(global_region_location))
        return;

    m_loaded_regions.ensure_region_available(global_region_location);
    Vector2I region_tile_offset
        {global_region_location.x*m_region_size_in_tiles.width,
         global_region_location.y*m_region_size_in_tiles.height};
    auto region_preparer = make_shared<MapRegionPreparer>(region_tile_offset);
    region_preparer->set_completer
        (MapRegionCompleter{global_region_location, m_loaded_regions});
    m_root_region->request_region_load(global_region_location, region_preparer, callbacks);
}
