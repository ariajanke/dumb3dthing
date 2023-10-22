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

#include "TileMapIdToSetMapping.hpp"
#include "TileSet.hpp"

namespace {

using namespace cul::exceptions_abbr;
using ConstTileSetPtr = TileSetMappingTile::ConstTileSetPtr;
using TileSetPtr      = TileSetMappingTile::TileSetPtr;

} // end of <anonymous> namespace

/* static */ std::vector<TileSetLayerWrapper>
    TileSetMappingLayer::make_views_from_sorted
    (const MappingContainer & container, const Size2I & grid_size)
{
    if (container.size() == 0) {
        return std::vector<TileSetLayerWrapper>{};
    } else if (container.size() == 1) {
        return std::vector<TileSetLayerWrapper>
            { TileSetLayerWrapper{container.begin(), container.end(), grid_size} };
    }
    std::vector<TileSetLayerWrapper> views;
    auto last = container.begin();
    for (auto itr = container.begin() + 1; itr != container.end(); ++itr) {
        if (last->same_tileset(*itr)) continue;
        views.emplace_back(TileSetLayerWrapper{last, itr, grid_size});
        last = itr;
    }
    views.emplace_back(TileSetLayerWrapper{last, container.end(), grid_size});
    return views;
}

// ----------------------------------------------------------------------------

/* static */ std::vector<TileLocation>
    TileSetMappingLayer::convert_to_tile_locations
    (const MappingView & tile_mappings)
{
    std::vector<TileLocation> rv;
    rv.reserve(tile_mappings.end() - tile_mappings.begin());
    for (auto & tile_mapping : tile_mappings)
        { rv.push_back(tile_mapping.to_tile_location()); }
    return rv;
}

// ----------------------------------------------------------------------------

TileSetMappingTile TileSetMappingTile::with_tileset
    (int tile_id_, const TileSetPtr & ptr) const
{
    auto tile_id_loc = ptr ? ptr->tile_id_location(tile_id_) : Vector2I{};
    return TileSetMappingTile{m_on_map, tile_id_loc, tile_id_, ptr};
}

TileLocation TileSetMappingTile::to_tile_location() const {
    TileLocation loc;
    loc.on_map = on_map();
    loc.on_tileset = on_tile_set();
    return loc;
}

// ----------------------------------------------------------------------------

/* static */ std::vector<TileSetMappingTile>
    TileMapIdToSetMapping_New::make_locations
    (const Size2I & size2)
{
    if (size2.width < 0 || size2.height < 0) {
        throw std::invalid_argument{""};
    }
    std::vector<TileSetMappingTile> tile_locations;
    tile_locations.reserve(size2.width*size2.height);
    for (int y = 0; y != size2.height; ++y) {
    for (int x = 0; x != size2.width ; ++x) {
        tile_locations.emplace_back
            (TileSetMappingTile::from_map_location(x, y));
    }}
    return tile_locations;
}

/* static */ std::vector<TileSetMappingTile>
    TileMapIdToSetMapping_New::clean_null_tiles
    (std::vector<TileSetMappingTile> && mapping_tiles)
{
    auto null_tile_id = [](const TileSetMappingTile & mapping_tile)
        { return !mapping_tile.has_tileset(); };
    auto rem_begin = std::remove_if
        (mapping_tiles.begin(), mapping_tiles.end(), null_tile_id);
    mapping_tiles.erase(rem_begin, mapping_tiles.end());
    return mapping_tiles;
}

TileMapIdToSetMapping_New::TileMapIdToSetMapping_New
    (std::vector<TileSetAndStartGid> && tilesets_and_starts):
    m_gid_map(std::move(tilesets_and_starts))
{
    std::sort(m_gid_map.begin(), m_gid_map.end(), order_by_gids);
    if (!m_gid_map.empty()) {
        const auto & last_entry = m_gid_map.back();
        m_gid_end = last_entry.start_gid + last_entry.tileset->total_tile_count();
    }
}

TileSetMappingLayer TileMapIdToSetMapping_New::
    make_mapping_for_layer(const Grid<int> & gid_layer)
{
    auto locations = make_locations(gid_layer.size2());
    for (auto & location : locations) {
        auto [tid, tileset] = map_id_to_set(gid_layer(location.on_map()));
        location = location.with_tileset(tid, tileset);
    }
    return TileSetMappingLayer
        {clean_null_tiles(std::move(locations)), gid_layer.size2()};
}

/* private */ Tuple<int, TileSetPtr> TileMapIdToSetMapping_New::map_id_to_set
    (int map_wide_id) const
{
    if (map_wide_id == 0) {
        return std::make_pair(0, nullptr);
    }
    if (map_wide_id < 1 || map_wide_id >= m_gid_end) {
        throw InvArg
            {"TileMapIdToSetMapping::map_id_to_set: Given map_id is either "
             "the empty tile or not contained in this map; translatable ids: "
             "[1 " + std::to_string(m_gid_end) + ")."};
    }
    TileSetAndStartGid samp;
    samp.start_gid = map_wide_id;
    auto itr = std::upper_bound
        (m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    assert(itr != m_gid_map.begin());
    --itr;
    assert(map_wide_id >= itr->start_gid);
    return std::make_pair(map_wide_id - itr->start_gid, itr->tileset);
}

/* private static */ bool TileMapIdToSetMapping_New::order_by_gids
    (const TileSetAndStartGid & lhs, const TileSetAndStartGid & rhs)
{ return lhs.start_gid < rhs.start_gid; }
