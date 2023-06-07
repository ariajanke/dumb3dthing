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

#include "MapRegion.hpp"
#include "RegionLoadRequest.hpp"

#include <unordered_map>
#include <iostream>

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

// This is just a set of callbacks, it does not collect across regions :/
class RegionLoadCollectorN { // <- this is going on a loader task
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorN() {}

    virtual void add_tiles
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset, const ProducableSubGrid &) = 0;
};

class RegionCollectionLoaderN final : public LoaderTask {
public:

    void operator () (Callbacks &) const final;
};

class MapRegionContainerN final {
public:
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;

    class RegionRefresh final {
    public:
        explicit RegionRefresh(bool & flag):
            m_flag(flag) {}

        void keep_this_frame() { m_flag = true; }

    private:
        bool & m_flag;
    };

    Optional<RegionRefresh> region_refresh_at(const Vector2I & r) {
        auto itr = m_loaded_regions.find(r);
        if (itr == m_loaded_regions.end()) return {};
        return RegionRefresh{itr->second.keep_on_refresh};
    }

    void decay_regions(TaskCallbacks & callbacks) {
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

    void set_region(const Vector2I & on_field_position,
                    const ViewGridTriangle & triangle_grid,
                    std::vector<Entity> && entities)
    {
        // y's are fine, loading x-ways not so
        auto * region = find(on_field_position);
        if (!region) {
            region = &m_loaded_regions[on_field_position];
        }
        region->teardown = std::make_shared<TeardownTask>
            (std::move(entities), triangle_grid.elements());
        region->keep_on_refresh = true;
        region->link_edge_container = InterTriangleLinkContainer{triangle_grid};
    }

private:
    struct LoadedMapRegion {
        InterTriangleLinkContainer link_edge_container;
        SharedPtr<TeardownTask> teardown;
        bool keep_on_refresh = true;
    };

    using LoadedRegionMap = std::unordered_map
        <Vector2I, LoadedMapRegion, Vector2IHasher>;

    LoadedMapRegion * find(const Vector2I & r) {
        auto itr = m_loaded_regions.find(r);
        return itr == m_loaded_regions.end() ? nullptr : &itr->second;
    }

    LoadedRegionMap m_loaded_regions;
};

class RegionLoadRequest;

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    MapRegionTracker() {}

    MapRegionTracker
        (UniquePtr<MapRegion> && root_region,
         const Size2I & region_size_in_tiles);

    explicit MapRegionTracker
        (UniquePtr<MapRegionN> && root_region):
        m_root_region_n(std::move(root_region)) {}

    void frame_refresh(TaskCallbacks & callbacks);

    void frame_hit(const Vector2I & global_region_location, TaskCallbacks & callbacks);

    void frame_hit(const RegionLoadRequest &, TaskCallbacks & callbacks);

    bool has_root_region() const noexcept
        { return !!m_root_region || m_root_region_n; }

private:
    MapRegionContainer m_loaded_regions;
    UniquePtr<MapRegion> m_root_region;
    Size2I m_region_size_in_tiles;

    MapRegionContainerN m_container_n;
    UniquePtr<MapRegionN> m_root_region_n;
};
