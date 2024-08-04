/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "../ProducableGroupFiller.hpp"
#include "SlopesTilesetTile.hpp"
#include "../../TriangleSegment.hpp"

#include <map>

class SlopesTilesetTile;
class MapTileset;

class WrappedCallbacksForSlopeTiles final : public ProducableTileCallbacks {
public:
    WrappedCallbacksForSlopeTiles
        (ProducableTileCallbacks & callbacks,
         const SharedPtr<const MapElementProperties> & props_ptr):
        m_wrapped(callbacks),
        m_layer_properties(props_ptr) {}
#   if 0
    StartingTupleBuilder add_entity() final {
        auto temp = m_wrapped.add_entity();
        auto & trans = temp.get<ModelTranslation>();
        trans.value += offset();
        return std::move(temp);
    }
#   endif
    Real next_random() final { return m_wrapped.next_random(); }

    AssetsRetrieval & assets_retrieval() const final
        { return m_wrapped.assets_retrieval(); }

    SharedPtr<RenderModel> make_render_model() final
        { return m_wrapped.make_render_model(); }

    ModelScale model_scale() const final
        { return m_wrapped.model_scale(); }

    ModelTranslation model_translation() const final
        { return ModelTranslation{m_wrapped.model_translation().value + offset()}; }

    Entity make_entity() final
        { return m_wrapped.make_entity(); }

private:
    void add_collidable_(const TriangleSegment & triangle) final {
        m_wrapped.add_collidable(triangle.move(offset()));
    }

    Vector offset() const;

    // :/
    mutable Optional<Vector> m_memoized_offset;
    ProducableTileCallbacks & m_wrapped;
    SharedPtr<const MapElementProperties> m_layer_properties;
};

// ----------------------------------------------------------------------------

class ProducableSlopesTile final : public ProducableTile {
public:
    ProducableSlopesTile() {}

    ProducableSlopesTile
        (const SharedPtr<const SlopesTilesetTile> & tileset_tile_ptr,
         const SharedPtr<const MapElementProperties> & layer_properties):
        m_tileset_tile_ptr(tileset_tile_ptr),
        m_layer_properties(layer_properties)
    { assert(m_layer_properties); }

    void set_neighboring_elevations(const NeighborCornerElevations & elvs) {
        m_elevations = elvs;
    }

    void operator () (ProducableTileCallbacks & callbacks) const final {
        if (m_tileset_tile_ptr) {
            WrappedCallbacksForSlopeTiles wrapped_callbacks
                {callbacks, m_layer_properties};
            m_tileset_tile_ptr->make
                (m_elevations, wrapped_callbacks);
        }
    }

private:
    const SlopesTilesetTile & verify_tile_ptr() const {
        if (m_tileset_tile_ptr)
            { return *m_tileset_tile_ptr; }
        throw RuntimeError
            {"Accessor not available without setting tileset tile pointer"};
    }

    SharedPtr<const SlopesTilesetTile> m_tileset_tile_ptr;
    SharedPtr<const MapElementProperties> m_layer_properties;
    NeighborCornerElevations m_elevations;
};

// ----------------------------------------------------------------------------

class SlopeGroupFiller final : public ProducableGroupFiller {
public:
    using TilesetTilePtr = SharedPtr<SlopesTilesetTile>;
    using TilesetTileMakerFunction = TilesetTilePtr(*)();
    using TilesetTileMakerMap = std::map<std::string, TilesetTileMakerFunction>;
    using TilesetTileGrid = Grid<TilesetTilePtr>;
    using TilesetTileGridPtr = SharedPtr<TilesetTileGrid>;

    static const TilesetTileMakerMap & builtin_makers();

    void make_group(CallbackWithCreator &) const final;

    void load
        (const MapTileset & map_tileset,
         PlatformAssetsStrategy & platform,
         const TilesetTileMakerMap & = builtin_makers());

private:
    TilesetTileGridPtr m_tileset_tiles;
};
