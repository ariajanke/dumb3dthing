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

    TriangleSegment transform(const TriangleSegment &) const;

    ModelScale model_scale() const;

    ModelTranslation model_translation() const;

    template <typename T>
    TilePositionFraming advance_with(ViewGridInserter<T> & inserter) const {
        inserter.advance();
        return TilePositionFraming
            {m_scale, m_on_field_region_position, m_scale(inserter.position())};
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

// tiled map
//   has a scale
// what is a tile?
// - actual geometry and visuals
//   - scaled by map alone
//   - positioned by map and framing device
// - map (sub) region
//   - scaled by map
//     - its geometry and visuals scaled by itself AND framing device
// positioning
// scale compounding
//
// On scaling?
//
// example (a)
// composite map scale factor of 4
// - has a map region scale factor of 2
//
// What is the scaling factor of the geometry?
//
// Each loaded in map sub region is a tile, therefore considered to be 4x4
// tiles.
// If the map region scale factor is 2. Each map region then would be 2x2.
// We can take the composite map region scaling factor and "divide" it by the
// local, map sub region. We would then get a correct scaling factor.
//
// example (b)
// composite map scale factor of 6
// - has a map region scale factor of 2
//
// Therefore we would like a set of 3x3 tiles of geometry/visuals in the map
// sub region.
//
// Additional conclusion
// - geometry/visuals ONLY consider the most local scaling factor
//
// On positioning?
//
// example (c)
// Map Region is placed at (1, 3)
// - has a map sub region, placed at tile position (2, 5)
//
// Where does the map sub region tiles start?
// It would be additive(?), therefore starts at (3, 8)
//
// On positioning AND scaling?
//
// example (d)
// Map Region is placed at (1, 3)
// - has a scaling factor of 6
// - has a map sub region, placed at tile position (2, 5)
//   - map sub region itself has a scale factor of 2
//
// What is both is scaling factor and starting position for the geometry and
// visuals on the map sub region?
//
// On positioning, start at (1, 3), moving to tile position (2, 5), with
// scaling factor 6. Each "tile" for the composite map is "6" units.
// So we start at position (1, 3) + (2, 5)*6 = (1, 3) + (12, 30) = (13, 33)
//
// On scaling, each map sub region "tile" is 2 units.
// Each composite map tile is 3x3 tiles of map sub region.
//
// example (e)
// From example (d), how do we find the starting position for geometry/visuals
// from the perspective of the map sub region, at its (1, 2) position?
//
// We know we start generally from (13, 33). Its own scaling factor is 2. To
// move to position (1, 2) would be (13, 33) + 2*(1, 2) = (13, 33) + (2, 4) =
// (15, 37).
//
// Additional conclusion:
// Tests should reflect exactly this.

class RegionPositionFraming final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionPositionFraming() {}

    // scale here is a ratio between a map's tile size and a
    // producable tile's size
    // scale doesn't belong here then, scaling is a property of the map

    explicit RegionPositionFraming
        (const ScaleComputation & tile_scale,
         const Vector2I & on_field_position = Vector2I{});

    bool operator == (const RegionPositionFraming &) const;

    SubRegionPositionFraming as_sub_region_framing() const;

    template <typename OverlapFuncT>
    void for_each_overlap(const Size2I & region_size,
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
