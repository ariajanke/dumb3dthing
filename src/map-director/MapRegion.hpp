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

#include "ProducableGrid.hpp"
#include "MapRegionContainer.hpp"

#include "../TriangleSegment.hpp"

#include <unordered_map>

class MapRegionPreparer;
class RegionLoadRequest;
class MapRegionContainer;
class ScaleComputation;
class RegionEdgeConnectionsAdder;

class TilePositionFraming final {
public:
    TilePositionFraming() {}

    TilePositionFraming
        (const ScaleComputation & scale,
         const Vector2I & on_field_position,
         const Vector2I & inserter_position = Vector2I{}):
        m_scale(scale),
        m_on_field_region_position(on_field_position),
        m_on_field_tile_position(on_field_position + inserter_position) {}

    TriangleSegment transform(const TriangleSegment & triangle) const
        { return triangle_transformation()(triangle); }

    ModelScale model_scale() const
        { return triangle_transformation().model_scale(); }

    ModelTranslation model_translation() const
        { return triangle_transformation().model_translation(); }

    template <typename T>
    TilePositionFraming advance_with(ViewGridInserter<T> & inserter) const {
        inserter.advance();
        return TilePositionFraming
            {m_scale, m_on_field_region_position, inserter.position()};
    }

private:
    TriangleSegmentTransformation triangle_transformation() const
        { return TriangleSegmentTransformation{m_scale, m_on_field_tile_position}; }

    ScaleComputation m_scale;
    Vector2I m_on_field_region_position;
    Vector2I m_on_field_tile_position;
};

class SubRegionPositionFraming final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    SubRegionPositionFraming() {}

    SubRegionPositionFraming
        (const ScaleComputation & scale,
         const Vector2I & on_field_position):
        m_scale(scale),
        m_on_field_position(on_field_position) {}

    TilePositionFraming tile_framing() const
        { return TilePositionFraming{m_scale, m_on_field_position}; }

    void set_containers_with
        (SharedPtr<ViewGridTriangle> && triangle_grid,
         std::vector<Entity> && entities,
         MapRegionContainer & container,
         RegionEdgeConnectionsAdder & edge_container_adder) const;

    auto region_refresh_for(MapRegionContainer & container) const
        { return container.region_refresh_at(m_on_field_position); }

private:
    ScaleComputation m_scale;
    Vector2I m_on_field_position;
};

class RegionPositionFraming final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionPositionFraming
        (const Vector2I & spawn_offset,
         const Vector2I & sub_region_position = Vector2I{}):
        m_spawn_offset(spawn_offset),
        m_sub_region_position(sub_region_position) {}

    template <typename OverlapFuncT>
    void for_each_overlap(const ScaleComputation & scale,
                          const RegionLoadRequest & request,
                          const Size2I & region_size,
                          OverlapFuncT && f) const;

private:
    struct OverlapFunc {
        virtual ~OverlapFunc() {}

        virtual void operator ()
            (const RectangleI & sub_region,
             const SubRegionPositionFraming &) const = 0;
    };

    void for_each_overlap_(const ScaleComputation & scale,
                           const RegionLoadRequest & request,
                           const Size2I & region_size,
                           const OverlapFunc & f) const;

    Vector2I m_spawn_offset;
    Vector2I m_sub_region_position;
};

template <typename OverlapFuncT>
void RegionPositionFraming::for_each_overlap
    (const ScaleComputation & scale,
     const RegionLoadRequest & request,
     const Size2I & region_size,
     OverlapFuncT && f) const
{
    class Impl final : public OverlapFunc {
    public:
        explicit Impl(OverlapFuncT && f): m_f(std::move(f)) {}

        void operator ()
            (const RectangleI & sub_region,
             const SubRegionPositionFraming & framing) const final
        { m_f(sub_region, framing); }

    private:
        OverlapFuncT m_f;
    };
    for_each_overlap_(scale, request, region_size, Impl{std::move(f)});
}

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) = 0;
};

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &) = 0;

    // can I send a request to only *part* of the map?
    virtual void process_limited_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) = 0;
};

class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion(ProducableTileViewGrid &&, ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &) final;

    void process_limited_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) final;

private:
    void collect_producables
        (const Vector2I & position_in_region,
         const Vector2I & offset,
         const Size2I & subgrid_size,
         RegionLoadCollectorBase &);

    void process_load_request_
        (ProducableTileViewGrid::SubGrid producables,
         const RegionLoadRequest &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &);

    Size2I region_size() const { return m_producables_view_grid.size2(); }

    // there's something that lives in here, but what?
    // something that breaks down into loadable "sub regions"
    ProducableTileViewGrid m_producables_view_grid;
    ScaleComputation m_scale;
};

// CompositeMapTileSet
// - each tile represents a region in a tiled map


class CompositeMapRegion final : public MapRegion {
public:

    void process_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &) final;

    void process_limited_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming &,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) final;
private:

    void collect_load_tasks
        (const Vector2I & position_in_region,
         const Vector2I & offset,
         const Size2I & subgrid_size,
         RegionLoadCollectorBase & collector);

    // not just MapRegions, but how to load them...
    // is it a view grid? not really
    // want to support 3D? nah, not for this "ticket"
    //
    // A tile is a chunk of map from the TiledMapRegion
    // Yet, only tilesets may speak of tiles in that fashion


};
