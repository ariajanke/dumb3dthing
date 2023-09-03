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

#include <unordered_map>

class MapRegionPreparer;
class RegionLoadRequest;
class MapRegionContainer;
class ScaleComputation;

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void collect_load_job
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset,
         const ProducableSubGrid &,
         const ScaleComputation &) = 0;
};

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) = 0;
};

class ScaleComputation final {
public:
    static Optional<ScaleComputation> parse(const char *);

    ScaleComputation() {}

    ScaleComputation(Real eastwest_factor,
                     Real updown_factor,
                     Real northsouth_factor);

    TriangleSegment operator () (const TriangleSegment &) const;

    Vector operator () (const Vector & r) const
        { return scale(r); }

    RectangleI operator () (const RectangleI & rect) const {
        auto scale_ = [] (Real x, int n)
            { return int(std::round(x*n)); };
        auto scale_x = [this, &scale_] (int n)
            { return scale_(m_factor.x, n); };
        auto scale_z = [this, &scale_] (int n)
            { return scale_(m_factor.z, n); };
        return RectangleI
            {scale_x(rect.left ), scale_z(rect.top   ),
             scale_x(rect.width), scale_z(rect.height)};
    }

    ModelScale to_model_scale() const;

private:
    static constexpr Vector k_no_scaling{1, 1, 1};

    Vector scale(const Vector &) const;

    Vector m_factor = k_no_scaling;
};

class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion(ProducableTileViewGrid &&, ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;

    // can I send a request to only *part* of the map?

private:
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
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;
private:

    // not just MapRegions, but how to load them...
    // is it a view grid? not really
    // want to support 3D? nah, not for this "ticket"
    //
    // A tile is a chunk of map from the TiledMapRegion
    // Yet, only tilesets may speak of tiles in that fashion


};
