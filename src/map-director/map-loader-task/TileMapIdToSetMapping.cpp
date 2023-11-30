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
#include "TilesetBase.hpp"

namespace {

using ConstTileSetPtr = TilesetMappingTile::ConstTileSetPtr;
using TileSetPtr      = TilesetMappingTile::TilesetPtr;

} // end of <anonymous> namespace

/* static */ bool TilesetMappingTile::less_than
    (const TilesetMappingTile & lhs, const TilesetMappingTile & rhs)
    { return lhs.m_tileset_ptr < rhs.m_tileset_ptr; }

/* static */ TilesetMappingTile TilesetMappingTile::from_map_location
    (int x_on_map, int y_on_map)
{
    return TilesetMappingTile
        {Vector2I{x_on_map, y_on_map}, Vector2I{}, 0, TileSetPtr{}};
}

TilesetMappingTile::TilesetMappingTile
    (const Vector2I & on_map,
     const Vector2I & on_tile_set,
     int tile_id_,
     const TilesetPtr & ptr):
    m_on_map(on_map),
    m_on_tile_set(on_tile_set),
    m_tile_id(tile_id_),
    m_tileset_ptr(ptr) {}

TilesetMappingTile TilesetMappingTile::with_tileset
    (int tile_id_, const TilesetPtr & ptr) const
{
    auto tile_id_loc = ptr ? ptr->tile_id_location(tile_id_) : Vector2I{};
    return TilesetMappingTile{m_on_map, tile_id_loc, tile_id_, ptr};
}

TileLocation TilesetMappingTile::to_tile_location() const {
    TileLocation loc;
    loc.on_map = on_map();
    loc.on_tileset = on_tile_set();
    return loc;
}

bool TilesetMappingTile::same_tileset(const TilesetMappingTile & rhs) const
    { return m_tileset_ptr == rhs.m_tileset_ptr; }

TilesetBase & TilesetMappingTile::tileset_of(const MappingView & view) const {
#   if MACRO_DEBUG
    assert(std::all_of(view.begin(), view.end(),
           [this](const TilesetMappingTile & mapping_tile)
           { return same_tileset(mapping_tile); }));
#   endif
    return *m_tileset_ptr;
}

// ----------------------------------------------------------------------------

/* static */ std::vector<TileLocation>
    TilesetMappingLayer::convert_to_tile_locations
    (const MappingView & tile_mappings)
{
    std::vector<TileLocation> rv;
    rv.reserve(tile_mappings.end() - tile_mappings.begin());
    for (auto & tile_mapping : tile_mappings)
        { rv.push_back(tile_mapping.to_tile_location()); }
    return rv;
}

/* static */ TilesetMappingLayer::MappingContainer
    TilesetMappingLayer::sort_container(MappingContainer && container)
{
    std::sort(container.begin(), container.end(),
              TilesetMappingTile::less_than);
    return std::move(container);
}

/* static */ std::vector<TilesetLayerWrapper>
    TilesetMappingLayer::make_views_from_sorted
    (const MappingContainer & container, const Size2I & grid_size)
{
    if (container.size() == 0) {
        return std::vector<TilesetLayerWrapper>{};
    } else if (container.size() == 1) {
        return std::vector<TilesetLayerWrapper>
            { TilesetLayerWrapper{container.begin(), container.end(), grid_size} };
    }
    std::vector<TilesetLayerWrapper> views;
    auto last = container.begin();
    for (auto itr = container.begin() + 1; itr != container.end(); ++itr) {
        if (last->same_tileset(*itr)) continue;
        views.emplace_back(TilesetLayerWrapper{last, itr, grid_size});
        last = itr;
    }
    views.emplace_back(TilesetLayerWrapper{last, container.end(), grid_size});
    return views;
}

/* static */ TilesetBase * TilesetMappingLayer::tileset_of
    (const MappingView & view)
{
    if (view.begin() == view.end()) return nullptr;
    return &view.begin()->tileset_of(view);
}

TilesetMappingLayer::TilesetMappingLayer
    (MappingContainer && locations, const Size2I & grid_size):
    m_locations(sort_container(std::move(locations))),
    m_mapping_views(make_views_from_sorted(m_locations, grid_size)) {}

// ----------------------------------------------------------------------------

/* static */ std::vector<TilesetMappingTile>
    TileMapIdToSetMapping::make_locations
    (const Size2I & size2)
{
    if (size2.width < 0 || size2.height < 0) {
        throw InvalidArgument{"Size fields must be non-negative integers"};
    }
    std::vector<TilesetMappingTile> tile_locations;
    tile_locations.reserve(size2.width*size2.height);
    for (int y = 0; y != size2.height; ++y) {
    for (int x = 0; x != size2.width ; ++x) {
        tile_locations.emplace_back
            (TilesetMappingTile::from_map_location(x, y));
    }}
    return tile_locations;
}

/* static */ std::vector<TilesetMappingTile>
    TileMapIdToSetMapping::clean_null_tiles
    (std::vector<TilesetMappingTile> && mapping_tiles)
{
    auto null_tile_id = [](const TilesetMappingTile & mapping_tile)
        { return !mapping_tile.has_tileset(); };
    auto rem_begin = std::remove_if
        (mapping_tiles.begin(), mapping_tiles.end(), null_tile_id);
    mapping_tiles.erase(rem_begin, mapping_tiles.end());
    return mapping_tiles;
}

TileMapIdToSetMapping::TileMapIdToSetMapping
    (std::vector<StartGidWithTileset> && tilesets_and_starts):
    m_gid_map(std::move(tilesets_and_starts))
{
    std::sort(m_gid_map.begin(), m_gid_map.end(), order_by_gids);
    if (!m_gid_map.empty()) {
        const auto & last_entry = m_gid_map.back();
        m_gid_end = last_entry.start_gid + last_entry.other->total_tile_count();
    }
}

TilesetMappingLayer TileMapIdToSetMapping::
    make_mapping_for_layer(const Grid<int> & gid_layer)
{
    auto locations = make_locations(gid_layer.size2());
    for (auto & location : locations) {
        auto [tid, tileset] = map_id_to_set(gid_layer(location.on_map()));
        location = location.with_tileset(tid, tileset);
    }
    return TilesetMappingLayer
        {clean_null_tiles(std::move(locations)), gid_layer.size2()};
}

/* private */ Tuple<int, TileSetPtr> TileMapIdToSetMapping::map_id_to_set
    (int map_wide_id) const
{
    if (map_wide_id == 0) {
        return std::make_pair(0, nullptr);
    }
    if (map_wide_id < 1 || map_wide_id >= m_gid_end) {
        throw InvalidArgument
            {"TileMapIdToSetMapping::map_id_to_set: Given map_id is either "
             "the empty tile or not contained in this map; translatable ids: "
             "[1 " + std::to_string(m_gid_end) + ")."};
    }
    StartGidWithTileset samp;
    samp.start_gid = map_wide_id;
    auto itr = std::upper_bound
        (m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    assert(itr != m_gid_map.begin());
    --itr;
    assert(map_wide_id >= itr->start_gid);
    return std::make_pair(map_wide_id - itr->start_gid, itr->other);
}

/* private static */ bool TileMapIdToSetMapping::order_by_gids
    (const StartGidWithTileset & lhs, const StartGidWithTileset & rhs)
{ return lhs.start_gid < rhs.start_gid; }
