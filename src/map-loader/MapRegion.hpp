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

class GridMapRegionCompleter {
public:
    virtual ~GridMapRegionCompleter() {}

    virtual void on_complete
        (const Vector2I & region_position,
         InterTriangleLinkContainer && link_container,
         SharedPtr<TeardownTask> && teardown_task) = 0;
};

class MapRegionCompleter final {
public:
    MapRegionCompleter() {}

    MapRegionCompleter
        (const Vector2I & region_position,
         GridMapRegionCompleter & grid_completer);

    template <typename ... Types>
    void on_complete(Types && ... args) const
        { verify_completer().on_complete(m_pos, std::forward<Types>(args)...); }

private:
    GridMapRegionCompleter & verify_completer() const;

    Vector2I m_pos;
    GridMapRegionCompleter * m_completer = nullptr;
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
        using Size2I           = cul::Size2<int>;
        using GridViewTriangleInserter = GridViewInserter<SharedPtr<TriangleLink>>;

        explicit EntityAndLinkInsertingAdder(const Size2I & grid_size):
            m_triangle_inserter(grid_size) {}

        void add_triangle(const TriangleSegment & triangle) final
            { m_triangle_inserter.push(triangle); }

        void add_entity(const Entity & ent) final
            { m_entities.push_back(ent); }

        std::vector<Entity> move_out_entities()
            { return std::move(m_entities); }

        void advance_grid_position()
            { m_triangle_inserter.advance(); }

        Tuple<GridViewTriangleInserter::ElementContainer,
              Grid<GridViewTriangleInserter::ElementView>>
            move_out_container_and_grid_view()
        {
            return m_triangle_inserter.
                transform_values<SharedPtr<TriangleLink>>(to_link).
                move_out_container_and_grid_view();
        }
    private:
        static SharedPtr<TriangleLink> to_link(const TriangleSegment & segment)
            { return make_shared<TriangleLink>(segment); }

        GridViewInserter<TriangleSegment> m_triangle_inserter;
        std::vector<Entity> m_entities;
    };

    explicit MapRegionPreparer(const Vector2I & tile_offset);

    void operator () (LoaderTask::Callbacks & callbacks) const final;

    void set_tile_factory_subgrid(TileFactorySubGrid && tile_factory_grid);

    void set_completer(const MapRegionCompleter &);

private:
    void finish_map
        (const TileFactorySubGrid & factory_grid, Callbacks & callbacks) const;

    TileFactorySubGrid m_tile_factory_grid;
    MapRegionCompleter m_completer;
    Vector2I m_tile_offset;
};
