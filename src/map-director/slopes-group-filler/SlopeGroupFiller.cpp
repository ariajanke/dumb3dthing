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

#include "SlopeGroupFiller.hpp"
#include "QuadBasedTilesetTile.hpp"
#include "RampPropertiesLoader.hpp"
#include "OutRampPropertiesLoader.hpp"
#include "InRampPropertiesLoader.hpp"
#include "WallTilesetTile.hpp"
#include "../slopes-group-filler.hpp"
#include "../MapTileset.hpp"
#include "NorthSouthSplit.hpp"
#include "InWallCornerSplits.hpp"
#include "OutWallCornerSplits.hpp"

namespace {

using TilesetTileMakerMap = SlopeGroupFiller::TilesetTileMakerMap;
using TilesetTileGridPtr = SlopeGroupFiller::TilesetTileGridPtr;
using TilesetTileGrid = SlopeGroupFiller::TilesetTileGrid;
using ProducableGroupCreation = ProducableGroupFiller::ProducableGroupCreation;

class NeighborElevationsComplete final :
    public NeighborCornerElevations::NeighborElevations
{
public:
    static Vector2I offset_for(CardinalDirection cd) {
        using Cd = CardinalDirection;
        switch (cd) {
        case Cd::north     : return Vector2I{ 0, -1};
        case Cd::east      : return Vector2I{ 1,  0};
        case Cd::south     : return Vector2I{ 0,  1};
        case Cd::west      : return Vector2I{-1,  0};
        case Cd::north_east: return Vector2I{ 1, -1};
        case Cd::north_west: return Vector2I{-1, -1};
        case Cd::south_east: return Vector2I{ 1,  1};
        case Cd::south_west: return Vector2I{-1,  1};
        }
        throw InvalidArgument{"invalid value for cardinal direction"};
    }

    NeighborElevationsComplete() {}

    explicit NeighborElevationsComplete
        (const Grid<TileCornerElevations> & producables_):
        m_producables(&producables_) {}

    TileCornerElevations elevations_from
        (const Vector2I & location_on_map, CardinalDirection cd) const final
    {
        auto r = location_on_map + offset_for(cd);
        if (!m_producables->has_position(r))
            { return TileCornerElevations{}; }
        return (*m_producables)(r);
    }

private:
    const Grid<TileCornerElevations> * m_producables = nullptr;
};

class SlopesGroupOwner final : public ProducableGroupOwner {
public:
    void set_tileset_tiles(const TilesetTileGridPtr & tileset_tiles) {
        m_tileset_tiles = tileset_tiles;
    }

    void reserve(std::size_t number_of_members, const Size2I & grid_size) {
        m_tileset_to_map_mapping.set_size(grid_size.width, grid_size.height);
        m_elevations_grid.set_size(grid_size.width, grid_size.height);
    }

    // yuck, this class should not be "multi-step" it means this class has more
    // than one responsibility
    void setup_elevations() {
        m_neighbor_elevations = NeighborElevationsComplete{m_elevations_grid};
        for (Vector2I r; r != m_elevations_grid.end_position(); r = m_elevations_grid.next(r)) {
            NeighborCornerElevations elvs;
            elvs.set_neighbors(r, m_neighbor_elevations);
            m_tileset_to_map_mapping(r).set_neighboring_elevations(elvs);
        }
    }

    ProducableTile & add_member(const TileLocation & tile_location) {
#       ifdef MACRO_DEBUG
        assert(m_tileset_tiles);
#       endif
        if (!m_tileset_tiles->has_position(tile_location.on_tileset) ||
            !m_tileset_to_map_mapping.has_position(tile_location.on_map))
        {
            throw InvalidArgument
                {"Cannot add member at specified location (grid not setup correctly?)"};
        }
        auto tileset_tile = (*m_tileset_tiles)(tile_location.on_tileset);
        if (tileset_tile) {
            m_elevations_grid(tile_location.on_map) =
                tileset_tile->corner_elevations();
        }
        return m_tileset_to_map_mapping(tile_location.on_map) =
            ProducableSlopesTile{tileset_tile};
    }

private:
    TilesetTileGridPtr m_tileset_tiles;
    Grid<ProducableSlopesTile> m_tileset_to_map_mapping;
    Grid<TileCornerElevations> m_elevations_grid;
    NeighborElevationsComplete m_neighbor_elevations;
};

class SlopesGroupCreator final : public ProducableGroupCreation {
public:
    void set_owner(SharedPtr<SlopesGroupOwner> && owner)
        { m_owner = std::move(owner); }

    void reserve
        (std::size_t number_of_members, const Size2I & grid_size) final
        { verify_owner_ptr().reserve(number_of_members, grid_size); }

    ProducableTile & add_member(const TileLocation & tile_location) final
        { return verify_owner_ptr().add_member(tile_location); }

    SharedPtr<ProducableGroupOwner> finish() final {
        (void)verify_owner_ptr();
        m_owner->setup_elevations();
        return std::move(m_owner);
    }

private:
    SlopesGroupOwner & verify_owner_ptr() {
        if (m_owner)
            { return *m_owner; }
        throw RuntimeError{"forgot to set slopes group owner pointer"};
    }

    SharedPtr<SlopesGroupOwner> m_owner;
};

using SlopesCreationFunction = SharedPtr<SlopesTilesetTile>(*)();

template <typename T>
SlopesCreationFunction make_slopes_creator() {
    return [] () -> SharedPtr<SlopesTilesetTile> { return make_shared<T>(); };
}

SharedPtr<SlopesTilesetTile> make_two_way_wall() {
    return make_shared<WallTilesetTile>
        (NorthSouthSplit::choose_geometry_strategy);
}

SharedPtr<SlopesTilesetTile> make_out_corner_wall() {
    return make_shared<WallTilesetTile>
        (NorthWestOutCornerSplit::choose_out_wall_strategy);
}

SharedPtr<SlopesTilesetTile> make_in_corner_wall() {
    return make_shared<WallTilesetTile>
        (NorthWestInCornerSplit::choose_in_wall_strategy);
}

template <QuadBasedTilesetTile::RampPropertiesLoaderStrategy kt_strat>
SharedPtr<SlopesTilesetTile> make_ramp() {
    return make_shared<QuadBasedTilesetTile>(kt_strat);
}

} // end of <anonymous> namespace

/* static */ const TilesetTileMakerMap & SlopeGroupFiller::builtin_makers() {
    using namespace slopes_group_filler_type_names;
    static TilesetTileMakerMap map {
        { k_flat    , make_ramp<FlatPropertiesLoader::instantiate_for> },
        { k_ramp    , make_ramp<RampPropertiesLoader::instantiate_for> },
        { k_out_ramp, make_ramp<OutRampPropertiesLoader::instantiate_for> },
        { k_in_ramp , make_ramp<InRampPropertiesLoader::instantiate_for> },
        { k_wall    , make_two_way_wall },
        { k_out_wall, make_out_corner_wall },
        { k_in_wall , make_in_corner_wall }
    };
    return map;
}

void SlopeGroupFiller::load
    (const MapTileset & map_tileset,
     PlatformAssetsStrategy & platform,
     const TilesetTileMakerMap & tileset_tile_makers)
{
    TilesetTileTexture tileset_tile_texture;
    tileset_tile_texture.load_texture(map_tileset, platform);
    m_tileset_tiles = make_shared<TilesetTileGrid>();
    m_tileset_tiles->set_size(map_tileset.size2().width, map_tileset.size2().height);
    for (Vector2I r; r != map_tileset.end_position(); r = map_tileset.next(r)) {
        auto * tileset_tile = map_tileset.tile_at(r);
        if (!tileset_tile)
            { continue; }
        auto itr = tileset_tile_makers.find(tileset_tile->type());
        if (itr == tileset_tile_makers.end())
            { continue; }
        tileset_tile_texture.set_texture_bounds(r);
        auto & created_tileset_tile = (*m_tileset_tiles)(r) = itr->second();
        created_tileset_tile->load(*tileset_tile, tileset_tile_texture, platform);
    }
}

void SlopeGroupFiller::make_group(CallbackWithCreator & callback) const {
    auto owner = make_shared<SlopesGroupOwner>();
    owner->set_tileset_tiles(m_tileset_tiles);
    SlopesGroupCreator creator;
    creator.set_owner(std::move(owner));
    callback(creator);
}
