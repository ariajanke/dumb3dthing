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

    MapRegionTracker() {}

    MapRegionTracker
        (UniquePtr<MapRegion> && root_region,
         const Size2I & region_size_in_tiles);

    void frame_refresh(TaskCallbacks & callbacks);

    void frame_hit(const Vector2I & global_region_location, TaskCallbacks & callbacks);

private:
#   if 0
    using NeighborRegions = std::array<MapRegionPreparer *, 4>;
#   endif
#   if 1

#   endif
#   if 0
    struct TrackerLoadedRegionN final {
        InterTriangleLinkContainer link_container;
        SharedPtr<
    };
#   endif


    class RegionContainer final : public MapRegionPreparer::GridRegionCompleter {
    public:
        void on_complete
            (const Vector2I & region_position,
             InterTriangleLinkContainer && link_container,
             SharedPtr<TeardownTask> && teardown_task) final
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
#       if 0
        LoadedMapRegion & operator [] (const Vector2I & r) {
            return m_loaded_regions[r];
        }
#       endif
        void ensure_region_available(const Vector2I & r)
            { (void)m_loaded_regions[r]; }


        bool find_and_keep(const Vector2I & r) {
            if (auto * region = find(r)) {
                return region->keep_on_refresh = true;
            }
            return false;
        }

        void frame_refresh(TaskCallbacks & callbacks) {
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

    private:
        class TrackerLoadedRegion final : public LoadedMapRegion {
        public:
            bool keep_on_refresh = true;
        };
        using LoadedRegionMap = std::unordered_map
            <Vector2I, TrackerLoadedRegion, Vector2IHasher>;

        TrackerLoadedRegion * find(const Vector2I & r) {
            auto itr = m_loaded_regions.find(r);
            if (itr == m_loaded_regions.end())
                return nullptr;
            return &itr->second;
        }


        LoadedRegionMap m_loaded_regions;
    };

#   if 0
    NeighborRegions get_neighbors_for(const Vector2I & r);

    LoadedMapRegion * location_to_pointer(const Vector2I & r);
#   endif
    RegionContainer m_loaded_regions;
#   if 0
    LoadedRegionMap m_loaded_regions;
#   endif
    UniquePtr<MapRegion> m_root_region;
    Size2I m_region_size_in_tiles;
};
