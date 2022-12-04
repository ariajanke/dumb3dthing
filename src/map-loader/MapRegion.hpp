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
    Size2I map_size_in_regions() const;

    Size2I m_region_size;
    TileFactoryGrid m_factory_grid;
};

/// all information pertaining to a loaded map region
///
/// (used by the region tracker)
class LoadedMapRegion {
public:
    InterTriangleLinkContainer link_edge_container;
    SharedPtr<TeardownTask> teardown;

protected:
    LoadedMapRegion() {}
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

    class GridRegionCompleter {
    public:
        virtual ~GridRegionCompleter() {}

        virtual void on_complete
            (const Vector2I & region_position,
             InterTriangleLinkContainer && link_container,
             SharedPtr<TeardownTask> && teardown_task) = 0;
    };

    class RegionCompleter final {
    public:
        RegionCompleter() {}

        RegionCompleter
            (const Vector2I & region_position,
             GridRegionCompleter & grid_completer):
            m_pos(region_position),
            m_completer(&grid_completer)
        {}

        template <typename ... Types>
        void on_complete(Types && ... args) const
            { verify_completer().on_complete(m_pos, std::forward<Types>(args)...); }

    private:
        GridRegionCompleter & verify_completer() const {
            using namespace cul::exceptions_abbr;
            if (m_completer) return *m_completer;
            throw RtError{""};
        }

        Vector2I m_pos;
        GridRegionCompleter * m_completer = nullptr;
    };
#   if 0
    using NeighborRegions = std::array<SharedPtr<MapRegionPreparer>, 4>;

    MapRegionPreparer
        (const LoadedMapRegion & target, Vector2I tile_offset,
         const NeighborRegions & neighbors);
#   endif
    explicit MapRegionPreparer(const Vector2I & tile_offset);

    void operator () (LoaderTask::Callbacks & callbacks) const final;

    void set_tile_factory_subgrid(TileFactorySubGrid && tile_factory_grid);

    void set_completer(const RegionCompleter &);
#   if 0
    void glue_triangles_to(const NeighborRegions &);

    void start_teardown_task(Callbacks &);
#   endif
private:
    void finish_map
        (const TileFactorySubGrid & factory_grid, Callbacks & callbacks) const;
#   if 0
    LoadedMapRegion & m_target_region;
#   endif
    TileFactorySubGrid m_tile_factory_grid;
#   if 0
    // LoadedMapRegion m_target_region;
    // | link container can just live here (and this belong to the region data
    // v as a shared ptr)
    InterTriangleLinkContainer m_link_edge_container;

    SharedPtr<TeardownTask> m_teardown;
#   endif
    RegionCompleter m_completer;
    Vector2I m_tile_offset;
#   if 0
    NeighborRegions m_neighbor_regions;
#   endif
};
