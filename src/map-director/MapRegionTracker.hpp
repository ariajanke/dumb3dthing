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
#include "RegionEdgeConnectionsContainer.hpp"

#include <unordered_map>

class RegionLoadCollector {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollector() {}

    virtual void add_tiles
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset, const ProducableSubGrid &) = 0;
};

class MapRegionContainer final {
public:
    using ViewGridTriangle = RegionEdgeLinksContainer::ViewGridTriangle;

    struct RegionDecayAdder {
        using TriangleLinkView = RegionEdgeLinksContainer::ViewGridTriangle;

        virtual ~RegionDecayAdder() {}

        virtual void add(const Vector2I & on_field_position,
                         const Size2I & grid_size,
                         std::vector<SharedPtr<TriangleLink>> && triangle_links,
                         std::vector<Entity> && entities) = 0;
    };

    class RegionRefresh final {
    public:
        explicit RegionRefresh(bool & flag):
            m_flag(flag) {}

        void keep_this_frame() { m_flag = true; }

    private:
        bool & m_flag;
    };

    Optional<RegionRefresh> region_refresh_at(const Vector2I & on_field_position) {
        auto itr = m_loaded_regions.find(on_field_position);
        if (itr == m_loaded_regions.end()) return {};
        return RegionRefresh{itr->second.keep_on_refresh};
    }

    void decay_regions(RegionDecayAdder & teardown_maker) {
        for (auto itr = m_loaded_regions.begin(); itr != m_loaded_regions.end(); ) {
            if (itr->second.keep_on_refresh) {
                itr->second.keep_on_refresh = false;
                ++itr;
            } else {
                teardown_maker.add(itr->first, itr->second.region_size,
                                   std::move(itr->second.triangle_links),
                                   std::move(itr->second.entities));
                itr = m_loaded_regions.erase(itr);
            }
        }
    }

    void set_region(const Vector2I & on_field_position,
                    const ViewGridTriangle & triangle_grid,
                    std::vector<Entity> && entities)
    {
        auto * region = find(on_field_position);
        if (!region) {
            region = &m_loaded_regions[on_field_position];
        }

        region->entities = std::move(entities);
        region->region_size = triangle_grid.size2();
        region->triangle_links.insert
            (region->triangle_links.end(),
             triangle_grid.elements().begin(),
             triangle_grid.elements().end());
        region->keep_on_refresh = true;
    }

private:
    struct LoadedMapRegion {
        Size2I region_size;
        std::vector<Entity> entities;
        std::vector<SharedPtr<TriangleLink>> triangle_links;
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

    explicit MapRegionTracker
        (UniquePtr<MapRegion> && root_region):
        m_root_region(std::move(root_region)) {}

    // should rename
    void frame_hit(const RegionLoadRequest &, TaskCallbacks & callbacks);

    bool has_root_region() const noexcept
        { return !!m_root_region; }

private:
    RegionEdgeConnectionsContainer m_edge_container;
    MapRegionContainer m_container;
    UniquePtr<MapRegion> m_root_region;
};
