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

#include "map-loader-helpers.hpp"

class MapRegionPreparer;

/// a map region is a grid of task pairs, one to load, one to teardown
class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void request_region_load
        (const Vector2I & local_position,
         const SharedPtr<MapRegionPreparer> & region_preparer, TaskCallbacks &) = 0;
};

/// a map region for tiled maps
class TiledMapRegion final : public MapRegion {
public:
    using Size2I = cul::Size2<int>;
    using Rectangle = cul::Rectangle<int>;

    TiledMapRegion
        (TileFactoryGrid && full_factory_grid,
         const Size2I & region_size_in_tiles):
         m_region_size(region_size_in_tiles),
         m_factory_grid(std::move(full_factory_grid)) {}

    void request_region_load
        (const Vector2I & local_region_position,
         const SharedPtr<MapRegionPreparer> & region_preparer,
         TaskCallbacks & callbacks) final;

private:
    Size2I m_region_size;
    TileFactoryGrid m_factory_grid;
};

/// all information pertaining to a loaded map region
///
/// (used by the region tracker)
struct LoadedMapRegion final {
    InterTriangleLinkContainer link_edge_container;
    SharedPtr<TeardownTask> teardown;
};

/// a loader task that prepares a region of the map
class MapRegionPreparer final : public LoaderTask {
public:
    class SlopesGridFromTileFactories final : public SlopesGridInterface {
    public:
        SlopesGridFromTileFactories(const TileFactorySubGrid & factory_subgrid):
            m_grid(factory_subgrid) {}

        Slopes operator () (Vector2I r) const final;

    private:
        const TileFactorySubGrid & m_grid;
    };

    class EntityAndLinkInsertingAdder final : public EntityAndTrianglesAdder {
    public:
        EntityAndLinkInsertingAdder
            (GridViewInserter<SharedPtr<TriangleLink>> & link_inserter_):
            m_link_inserter(link_inserter_) {}

        void add_triangle(const TriangleSegment & triangle) final
            { m_link_inserter.push(make_shared<TriangleLink>(triangle)); }

        void add_entity(const Entity & ent) final
            { m_entities.push_back(ent); }

        std::vector<Entity> move_out_entities()
            { return std::move(m_entities); }

    private:
        GridViewInserter<SharedPtr<TriangleLink>> & m_link_inserter;
        std::vector<Entity> m_entities;
    };
    using NeighborRegions = std::array<LoadedMapRegion *, 4>;

    MapRegionPreparer
        (LoadedMapRegion & target, Vector2I tile_offset,
         const NeighborRegions & neighbors);

    void operator () (LoaderTask::Callbacks & callbacks) const final;

    void set_tile_factory_subgrid(TileFactorySubGrid && tile_factory_grid);

private:
    void finish_map
        (const TileFactorySubGrid & factory_grid, LoadedMapRegion & target,
         Callbacks & callbacks) const;

    TileFactorySubGrid m_tile_factory_grid;
    LoadedMapRegion & m_target_region;
    Vector2I m_tile_offset;
    NeighborRegions m_neighbor_regions;
};
