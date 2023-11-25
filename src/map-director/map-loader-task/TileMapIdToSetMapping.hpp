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

#include "../../Definitions.hpp"
#include "../ProducableGrid.hpp"

#include "TilesetLoadingTask.hpp"

class TilesetBase;
struct TileLocation;

class TilesetMappingTile final {
public:
    using ConstTileSetPtr = SharedPtr<const TilesetBase>;
    using TilesetPtr      = SharedPtr<TilesetBase>;
    using MappingContainer = std::vector<TilesetMappingTile>;
    using MappingContainerIterator = MappingContainer::const_iterator;
    using MappingView = View<MappingContainerIterator>;

    static bool less_than
        (const TilesetMappingTile &, const TilesetMappingTile &);

    static TilesetMappingTile from_map_location(int x_on_map, int y_on_map);

    TilesetMappingTile() {}

    TilesetMappingTile
        (const Vector2I & on_map,
         const Vector2I & on_tile_set,
         int tile_id_,
         const TilesetPtr & ptr);

    Vector2I on_map() const { return m_on_map; }

    Vector2I on_tile_set() const { return m_on_tile_set; }

    int tile_id() const { return m_tile_id; }

    TilesetMappingTile with_tileset
        (int tile_id_, const TilesetPtr & ptr) const;

    bool same_tileset(const TilesetMappingTile &) const;

    TileLocation to_tile_location() const;

    bool has_tileset() const { return static_cast<bool>(m_tileset_ptr); }

    TilesetBase & tileset_of(const MappingView &) const;

private:
    Vector2I m_on_map;
    Vector2I m_on_tile_set;
    int m_tile_id;
    TilesetPtr m_tileset_ptr;
};

// ----------------------------------------------------------------------------

class TilesetLayerWrapper;

class TilesetMappingLayer final {
public:
    using MappingContainer = TilesetMappingTile::MappingContainer;
    using MappingContainerIterator = TilesetMappingTile::MappingContainerIterator;
    using MappingView = TilesetMappingTile::MappingView;
    using MappingViewIterator = std::vector<MappingView>::const_iterator;

    static std::vector<TileLocation> convert_to_tile_locations
        (const MappingView & tile_mappings);

    static MappingContainer sort_container(MappingContainer &&);

    static std::vector<TilesetLayerWrapper> make_views_from_sorted
        (const MappingContainer & container, const Size2I & grid_size);

    static TilesetBase * tileset_of(const MappingView &);

    TilesetMappingLayer
        (MappingContainer && locations, const Size2I & grid_size);

    auto begin() const { return m_mapping_views.begin(); }

    auto end() const { return m_mapping_views.end(); }

private:
    MappingContainer m_locations;
    std::vector<TilesetLayerWrapper> m_mapping_views;
};

// ----------------------------------------------------------------------------

class TilesetLayerWrapper final {
public:
    using MappingContainerIterator = TilesetMappingLayer::MappingContainerIterator;
    using MappingView = TilesetMappingLayer::MappingView;

    TilesetLayerWrapper
        (const MappingContainerIterator & begin,
         const MappingContainerIterator & end,
         const Size2I & grid_size):
        m_view(begin, end), m_grid_size(grid_size) {}

    MappingContainerIterator begin() const { return m_view.begin(); }

    MappingContainerIterator end() const { return m_view.end(); }

    const Size2I & grid_size() const { return m_grid_size; }

    const MappingView & as_view() const { return m_view; }

private:
    MappingView m_view;
    Size2I m_grid_size;
};

// ----------------------------------------------------------------------------

class TileMapIdToSetMapping final {
public:
    using StartGidWithTileset = StartGidWith<SharedPtr<TilesetBase>>;

    static std::vector<TilesetMappingTile> make_locations(const Size2I &);

    static std::vector<TilesetMappingTile> clean_null_tiles
        (std::vector<TilesetMappingTile> &&);

    TileMapIdToSetMapping() {}

    explicit TileMapIdToSetMapping(std::vector<StartGidWithTileset> &&);

    TilesetMappingLayer make_mapping_for_layer(const Grid<int> &);

private:
    using ConstTileSetPtr = TilesetMappingTile::ConstTileSetPtr;
    using TileSetPtr      = TilesetMappingTile::TilesetPtr;

    static bool order_by_gids(const StartGidWithTileset &, const StartGidWithTileset &);

    Tuple<int, TileSetPtr> map_id_to_set(int map_wide_id) const;

    std::vector<StartGidWithTileset> m_gid_map;
    int m_gid_end = 0;
};
