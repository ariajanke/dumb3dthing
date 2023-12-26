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

#include "SlopeGroupFillerN.hpp"
#include "FlatTilesetTileN.hpp"
#include "RampTilesetTileN.hpp"
#include "../slopes-group-filler.hpp"
#include "../MapTileset.hpp"

namespace {

using TilesetTileMakerMap = SlopeGroupFiller::TilesetTileMakerMap;
using TilesetTileGridPtr = SlopeGroupFiller::TilesetTileGridPtr;
using TilesetTileGrid = SlopeGroupFiller::TilesetTileGrid;
using ProducableGroupCreation = ProducableGroupFiller::ProducableGroupCreation;

class NeighborElevationsComplete final :
    public TileCornerElevations::NeighborElevations
{
public:
    static Vector2I offset_for(CardinalDirection cd) {
        using Cd = CardinalDirection;
        switch (cd) {
        case Cd::n : return Vector2I{ 0, -1};
        case Cd::e : return Vector2I{-1,  0};
        case Cd::s : return Vector2I{ 0,  1};
        case Cd::w : return Vector2I{ 1,  0};
        case Cd::ne: return Vector2I{-1, -1};
        case Cd::nw: return Vector2I{ 1, -1};
        case Cd::se: return Vector2I{-1,  1};
        case Cd::sw: return Vector2I{ 1,  1};
        }
        throw InvalidArgument{"invalid value for cardinal direction"};
    }

    NeighborElevationsComplete
        (const Grid<ProducableSlopesTile> & producables_,
         const Vector2I & position):
        m_producables(producables_),
        m_position(position) {}

    TileCornerElevations elevations_from(CardinalDirection cd) const final {
        auto r = m_position + offset_for(cd);
        if (!m_producables.has_position(r))
            { return TileCornerElevations{}; }
        return m_producables(r).corner_elevations();
    }

private:
    const Grid<ProducableSlopesTile> & m_producables;
    Vector2I m_position;
};

class SlopesGroupOwner final : public ProducableGroupOwner {
public:
    void set_tileset_tiles(const TilesetTileGridPtr & tileset_tiles) {
        m_tileset_tiles = tileset_tiles;
    }

    void reserve(std::size_t number_of_members, const Size2I & grid_size) {
        m_tileset_to_map_mapping.set_size(grid_size.width, grid_size.height);
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
        NeighborElevationsComplete neighbor_elevations
            {m_tileset_to_map_mapping, tile_location.on_map};
        return m_tileset_to_map_mapping(tile_location.on_map) =
            ProducableSlopesTile
            {(*m_tileset_tiles)(tile_location.on_tileset),
             neighbor_elevations.elevations()};
    }

private:
    TilesetTileGridPtr m_tileset_tiles;
    Grid<ProducableSlopesTile> m_tileset_to_map_mapping;
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

} // end of <anonymous> namespace

/* static */ const TilesetTileMakerMap & SlopeGroupFiller::builtin_makers() {
    using namespace slopes_group_filler_type_names;
    using Rt = SharedPtr<SlopesTilesetTile>;
    static TilesetTileMakerMap map {
        { k_flat, [] () -> Rt { return make_shared<FlatTilesetTile>(); } },
        { k_ramp, [] () -> Rt { return make_shared<RampTileseTile>(); } }
    };
    return map;
}

void SlopeGroupFiller::load
    (const MapTileset & map_tileset,
     PlatformAssetsStrategy & platform,
     const TilesetTileMakerMap & tileset_tile_makers)
{
    m_tileset_tiles = make_shared<TilesetTileGrid>();
    m_tileset_tiles->set_size(map_tileset.size2().width, map_tileset.size2().height);
    for (Vector2I r; r != map_tileset.end_position(); r = map_tileset.next(r)) {
        auto & properties = xml_grid(r);
        auto itr = tileset_tile_makers.find(properties.type());
        if (itr == tileset_tile_makers.end())
            { continue; }
        auto & created_tileset_tile = (*m_tileset_tiles)(r) = itr->second();
        created_tileset_tile->load(xml_grid, r, platform);
    }
}

void SlopeGroupFiller::make_group(CallbackWithCreator & callback) const {
    auto owner = make_shared<SlopesGroupOwner>();
    owner->set_tileset_tiles(m_tileset_tiles);
    SlopesGroupCreator creator;
    creator.set_owner(std::move(owner));
    callback(creator);
}
