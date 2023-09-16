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

class RegionEdgeConnectionsAdder;
class RegionLoadRequest;

class TilePositionFraming final {
public:
    TilePositionFraming() {}

    TilePositionFraming
        (const ScaleComputation & scale,
         const Vector2I & on_field_position,
         const Vector2I & inserter_position = Vector2I{});

    TriangleSegment transform(const TriangleSegment &) const;

    ModelScale model_scale() const;

    ModelTranslation model_translation() const;

    template <typename T>
    TilePositionFraming advance_with(ViewGridInserter<T> & inserter) const {
        inserter.advance();
        return TilePositionFraming
            {m_scale, m_on_field_region_position, inserter.position()};
    }

private:
    TriangleSegmentTransformation triangle_transformation() const;

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

    bool operator == (const SubRegionPositionFraming & rhs) const {
        return m_scale == rhs.m_scale &&
               m_on_field_position == rhs.m_on_field_position;
    }

private:
    ScaleComputation m_scale;
    Vector2I m_on_field_position;
};

class RegionPositionFraming final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionPositionFraming() {}

    // scale here is a ratio between a map's tile size and a
    // producable tile's size

    RegionPositionFraming
        (const ScaleComputation & tile_scale,
         const Size2I & region_size_in_tiles,
         const Vector2I & on_field_position = Vector2I{});

    template <typename OverlapFuncT>
    void for_each_overlap(const RegionLoadRequest & request,
                          OverlapFuncT && f) const;

    RegionPositionFraming overlay_with
        (const ScaleComputation & tile_scale,
         const Size2I & region_size_in_tiles,
         const Vector2I & on_field_position = Vector2I{}) const;

    RegionPositionFraming move(const Vector2I & map_tile_position) const;

    SubRegionPositionFraming as_sub_region_framing() const;

    bool operator == (const RegionPositionFraming &) const;

private:
    struct OverlapFunc {
        virtual ~OverlapFunc() {}

        virtual void operator ()
            (const RegionPositionFraming & sub_frame,
             const RectangleI & sub_region_size_in_tiles) const = 0;
    };

    void for_each_overlap_(const RegionLoadRequest & request,
                           const OverlapFunc & f) const;

    Vector2I m_on_field_position;
    ScaleComputation m_tile_scale;
    Size2I m_region_size_in_tiles;
};

template <typename OverlapFuncT>
void RegionPositionFraming::for_each_overlap
    (const RegionLoadRequest & request, OverlapFuncT && f) const
{
    class Impl final : public OverlapFunc {
    public:
        explicit Impl(OverlapFuncT && f): m_f(std::move(f)) {}

        void operator ()
            (const RegionPositionFraming & sub_frame,
             const RectangleI & sub_region_size_in_tiles) const final
        { m_f(sub_frame, sub_region_size_in_tiles); }

    private:
        OverlapFuncT m_f;
    };
    for_each_overlap_(request, Impl{std::move(f)});
}
