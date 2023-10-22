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

class TileSet;
class ProducableGroupFiller;

/// Translates global ids to tileset ids, along with their tilesets
///
/// Can also be used as an owner for tilesets (needs to for translation to
/// work). The tilesets maybe moved out, however this empties the translator.
///   TileMapIdToSetMapping
///   MapToSetIdMapping
class TileMapIdToSetMapping final :
    public UnfinishedProducableTileViewGrid::ProducableGroupFillerExtraction
{
public:
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;

    struct TileSetAndStartGid final {
        TileSetAndStartGid() {}

        TileSetAndStartGid(TileSetPtr && tileset_, int start_gid_):
            tileset(std::move(tileset_)),
            start_gid(start_gid_)
        {}

        TileSetPtr tileset;
        int start_gid = 0;
    };

    TileMapIdToSetMapping() {}

    explicit TileMapIdToSetMapping(std::vector<TileSetAndStartGid> &&);

    Tuple<int, ConstTileSetPtr> map_id_to_set(int map_wide_id) const;

    [[nodiscard]] std::vector<SharedPtr<const ProducableGroupFiller>>
        move_out_fillers() final;

    void swap(TileMapIdToSetMapping &);

private:
    static bool order_by_gids(const TileSetAndStartGid &, const TileSetAndStartGid &);

    [[nodiscard]] std::vector<SharedPtr<TileSet>> move_out_tilesets();

    std::vector<TileSetAndStartGid> m_gid_map;
    int m_gid_end = 0;
};

class TileSetMapElementVisitor;
#if 0
class TileSetIdGrid final {
public:
    TileSetIdGrid();

    TileSetIdGrid
        (const Grid<int> & gid_layer, int start_gid, int end_gid);

    // tid could possibly be zero in the data?
    // perhaps take negative to mean that there's nothing there?
    // this needs to be thought out at the time where we actually handle the
    // tid
    int to_tileset_id(Vector2I r) const;

    // wait and see on iterating this

private:
    const Grid<int> * m_gid_layer = nullptr;
    int m_start_gid = 0, m_gid_end = 0;
};
#endif
class TileMapIdToSetMappingElement final {
public:

    void add_map_elements(TileSetMapElementVisitor & visitor) const;
};
#if 0
class TileMapIdToSetMappingIterator final {
public:
    using TileSetAndStartGid = TileMapIdToSetMapping::TileSetAndStartGid;
    // has to be a writable because we're consuming the thing :/
    using IteratorImpl = std::vector<TileSetAndStartGid>::iterator;

    explicit TileMapIdToSetMappingIterator(IteratorImpl);

    TileMapIdToSetMappingIterator(const Grid<int> *, IteratorImpl);

    TileMapIdToSetMappingElement & operator ++ ();

    TileMapIdToSetMappingElement operator * () const;

    TileMapIdToSetMappingElement * operator -> () const;

    bool operator == (const TileMapIdToSetMappingIterator &) const;

private:
    IteratorImpl m_position;
    TileMapIdToSetMappingElement m_element;
};
#endif
#if 0
struct TileLocationWithId final {
    Vector2I on_map_location;
    Vector2I on_tile_set_location;
    int tile_id = 0;
};
#endif

struct TileLocation;

class TileSetMappingTile final {
public:
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using MappingContainer = std::vector<TileSetMappingTile>;
    using MappingContainerIterator = MappingContainer::const_iterator;
    using MappingView = View<MappingContainerIterator>;

    static bool less_than
        (const TileSetMappingTile & lhs, const TileSetMappingTile & rhs)
        { return lhs.m_tileset_ptr < rhs.m_tileset_ptr; }

    static TileSetMappingTile from_map_location(int x_on_map, int y_on_map) {
        return TileSetMappingTile
            {Vector2I{x_on_map, y_on_map}, Vector2I{}, 0, ConstTileSetPtr{}};
    }


    TileSetMappingTile() {}

    TileSetMappingTile
        (const Vector2I & on_map,
         const Vector2I & on_tile_set,
         int tile_id_,
         const ConstTileSetPtr & ptr):
        m_on_map(on_map),
        m_on_tile_set(on_tile_set),
        m_tile_id(tile_id_),
        m_tileset_ptr(ptr) {}

    Vector2I on_map() const { return m_on_map; }

    Vector2I on_tile_set() const { return m_on_tile_set; }

    int tile_id() const { return m_tile_id; }

    TileSetMappingTile with_tileset
        (int tile_id_, const ConstTileSetPtr & ptr) const;

    bool same_tileset(const TileSetMappingTile & rhs) const
        { return m_tileset_ptr == rhs.m_tileset_ptr; }

    TileLocation to_tile_location() const;

    const TileSet & tileset_of(const MappingView & view) const {
#       if MACRO_DEBUG
        assert(std::all_of(view.begin(), view.end(),
               [this](const TileSetMappingTile & mapping_tile)
               { same_tileset(mapping_tile); }));
#       endif
        return *m_tileset_ptr;
    }

private:
    Vector2I m_on_map;
    Vector2I m_on_tile_set;
    int m_tile_id;
    ConstTileSetPtr m_tileset_ptr;
};

class TileSetLayerWrapper;

class TileSetMappingLayer final {
public:
    using MappingContainer = TileSetMappingTile::MappingContainer;
    using MappingContainerIterator = TileSetMappingTile::MappingContainerIterator;
    using MappingView = TileSetMappingTile::MappingView;
    using MappingViewIterator = std::vector<MappingView>::const_iterator;

    static std::vector<TileLocation> convert_to_tile_locations
        (const MappingView & tile_mappings);

    static MappingContainer sort_container(MappingContainer && container) {
        std::sort(container.begin(), container.end(),
                  TileSetMappingTile::less_than);
        return std::move(container);
    }

    static std::vector<TileSetLayerWrapper> make_views_from_sorted
        (const MappingContainer & container, const Size2I & grid_size);

    static const TileSet * tileset_of(const MappingView & view) {
        if (view.begin() == view.end()) return nullptr;
        return &view.begin()->tileset_of(view);
    }

    TileSetMappingLayer
        (MappingContainer && locations, const Size2I & grid_size):
        m_locations(sort_container(std::move(locations))),
        m_mapping_views(make_views_from_sorted(m_locations, grid_size))
    {}

    auto begin() const { return m_mapping_views.begin(); }

    auto end() const { return m_mapping_views.end(); }

private:
    MappingContainer m_locations;
    std::vector<TileSetLayerWrapper> m_mapping_views;
};

class TileSetLayerWrapper final {
public:
    using MappingContainerIterator = TileSetMappingLayer::MappingContainerIterator;
    using MappingView = TileSetMappingLayer::MappingView;

    TileSetLayerWrapper
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

class TileMapIdToSetMapping_New final {
public:
    using TileSetAndStartGid = TileMapIdToSetMapping::TileSetAndStartGid;

    static std::vector<TileSetMappingTile> make_locations(const Size2I &);

    TileMapIdToSetMapping_New() {}

    explicit TileMapIdToSetMapping_New(std::vector<TileSetAndStartGid> &&);

    TileSetMappingLayer make_mapping_for_layer(const Grid<int> &);

private:
    using ConstTileSetPtr = TileSetMappingTile::ConstTileSetPtr;

    static bool order_by_gids(const TileSetAndStartGid &, const TileSetAndStartGid &);

    Tuple<int, ConstTileSetPtr> map_id_to_set(int map_wide_id) const;

    std::vector<TileSetAndStartGid> m_gid_map;
    int m_gid_end = 0;
};
