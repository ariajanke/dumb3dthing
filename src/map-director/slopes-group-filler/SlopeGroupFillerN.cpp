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
#include "../slopes-group-filler.hpp"

namespace {

using TilesetTileMakerMap = SlopeGroupFiller::TilesetTileMakerMap;
using TilesetTileGridPtr = SlopeGroupFiller::TilesetTileGridPtr;

class SlopesGroupOwner final : public ProducableGroupOwner {
public:
    void reserve(std::size_t number_of_members, const Size2I & grid_size) {
        m_slope_tiles.reserve(number_of_members);
        m_tileset_to_map_mapping =
            make_shared<Grid<SlopesBasedTileFactory *>>();
        m_tileset_to_map_mapping->set_size(grid_size, nullptr);
    }

    ProducableTile & add_member(const TileLocation & tile_location) {
#       ifdef MACRO_DEBUG
        assert(m_slope_tiles.capacity() > m_slope_tiles.size());
#       endif
        m_slope_tiles.emplace_back
            (ProducableSlopeTile{tile_location.on_map, m_tileset_to_map_mapping});
        (*m_tileset_to_map_mapping)(tile_location.on_map) =
            m_tile_factory_grid(tile_location.on_tileset).get();
        return m_slope_tiles.back();
    }

    void set_tile_factory_grid(const TileFactoryGrid & grid) {
        m_tile_factory_grid = grid;
    }

private:
    SharedPtr<Grid<SlopesBasedTileFactory *>> m_tileset_to_map_mapping;
    std::vector<ProducableSlopeTile> m_slope_tiles;
    TileFactoryGrid m_tile_factory_grid;
};

} // end of <anonymous> namespace

/* static */ const TilesetTileMakerMap & SlopeGroupFiller::builtin_makers() {
    using namespace slopes_group_filler_type_names;
    using Rt = SharedPtr<SlopesTilesetTile>;
    static TilesetTileMakerMap map {
        { k_flat, [] () -> Rt { return make_shared<FlatTilesetTile>(); } }
    };
    return map;
}

void SlopeGroupFiller::load
    (const TilesetXmlGrid & xml_grid,
     PlatformAssetsStrategy & platform,
     const TilesetTileMakerMap & tileset_tile_makers)
{
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        auto & properties = xml_grid(r);
        auto itr = tileset_tile_makers.find(properties.type());
        if (itr == tileset_tile_makers.end())
            { continue; }
        itr->second();
    }
}

void SlopeGroupFiller::make_group(CallbackWithCreator &) const {

}
