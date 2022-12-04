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

#include <unordered_map>

class MapRegionContainer final : public GridMapRegionCompleter {
public:
    void on_complete
        (const Vector2I & region_position,
         InterTriangleLinkContainer && link_container,
         SharedPtr<TeardownTask> && teardown_task) final;

    void ensure_region_available(const Vector2I & r);

    bool find_and_keep(const Vector2I & r);

    void frame_refresh(TaskCallbacks & callbacks);

private:
    struct LoadedMapRegion {
        InterTriangleLinkContainer link_edge_container;
        SharedPtr<TeardownTask> teardown;
        bool keep_on_refresh = true;
    };
    using LoadedRegionMap = std::unordered_map
        <Vector2I, LoadedMapRegion, Vector2IHasher>;

    LoadedMapRegion * find(const Vector2I & r);

    LoadedRegionMap m_loaded_regions;
};

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
    MapRegionContainer m_loaded_regions;
    UniquePtr<MapRegion> m_root_region;
    Size2I m_region_size_in_tiles;
};
