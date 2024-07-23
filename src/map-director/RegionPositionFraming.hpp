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
class RegionLoadRequestBase;

class TilePositionFraming final {
public:
    TilePositionFraming() {}

    TilePositionFraming
        (const ScaleComputation & scale,
         const Vector2I & on_field_position,
         const Vector2I & inserter_position = Vector2I{});

    template <typename T>
    TilePositionFraming advance_with(ViewGridInserter<T> &) const;

    ModelScale model_scale() const;

    ModelTranslation model_translation() const;

    TriangleSegment transform(const TriangleSegment &) const;

    template <typename T>
    void call_seed_on(T & obj) {
        auto data =
            reinterpret_cast<const unsigned &>(m_on_field_region_position.x) << 10 ^
            reinterpret_cast<const unsigned &>(m_on_field_region_position.y) << 7 ^
            reinterpret_cast<const unsigned &>(m_on_field_tile_position.x) << 3 ^
            reinterpret_cast<const unsigned &>(m_on_field_tile_position.y) ^
            ~0u;
        obj.seed(data);
    }

private:
    Vector translation() const;

    ScaleComputation m_scale;
    Vector2I m_on_field_region_position;
    Vector2I m_on_field_tile_position;
};

// ----------------------------------------------------------------------------

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

    bool operator == (const SubRegionPositionFraming &) const;

private:
    ScaleComputation m_scale;
    Vector2I m_on_field_position;
};

// ----------------------------------------------------------------------------

class RegionPositionFraming final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionPositionFraming() {}

    explicit RegionPositionFraming
        (const ScaleComputation & scale_map_tile_over_producable_tile,
         const Vector2I & on_field_position = Vector2I{});

    bool operator == (const RegionPositionFraming &) const;

    SubRegionPositionFraming as_sub_region_framing() const;

    template <typename OverlapFuncT>
    void for_each_overlap
        (const Size2I & region_size,
         const RegionLoadRequestBase & request,
         OverlapFuncT && f) const;

    RegionPositionFraming move(const Vector2I & map_tile_position) const;

    RegionPositionFraming with_scaling(const ScaleComputation & map_scale) const;

private:
    struct OverlapFunc {
        virtual ~OverlapFunc() {}

        virtual void operator ()
            (const RegionPositionFraming & sub_frame,
             const RectangleI & sub_region_size_in_tiles) const = 0;
    };

    void for_each_overlap_(const Size2I & region_size,
                           const RegionLoadRequestBase & request,
                           const OverlapFunc & f) const;

    Vector2I m_on_field_position;
    ScaleComputation m_tile_scale;
};

// ----------------------------------------------------------------------------

template <typename T>
TilePositionFraming TilePositionFraming::advance_with
    (ViewGridInserter<T> & inserter) const
{
    inserter.advance();
    return TilePositionFraming
        {m_scale, m_on_field_region_position, m_scale.of(inserter.position())};
}

// ----------------------------------------------------------------------------

template <typename OverlapFuncT>
void RegionPositionFraming::for_each_overlap
    (const Size2I & region_size,
     const RegionLoadRequestBase & request,
     OverlapFuncT && f) const
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
    for_each_overlap_(region_size, request, Impl{std::move(f)});
}
