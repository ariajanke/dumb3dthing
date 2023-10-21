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

#include "../ParseHelpers.hpp"
#include "../ProducableGroupFiller.hpp"
#include "../MapRegion.hpp"

#include <map>

class TileSetXmlGrid;

class StackableProducableTileGrid final {
public:
    using ProducableGroupCollection = std::vector<SharedPtr<ProducableGroup_>>;
    using ProducableGroupCollectionPtr = SharedPtr<ProducableGroupCollection>;

    StackableProducableTileGrid
        (Grid<ProducableTile *> && producables,
         const ProducableGroupCollectionPtr & producable_owners);

    StackableProducableTileGrid stack_with(StackableProducableTileGrid &&);

    ProducableTileViewGrid to_producables() {

    }

private:
    std::vector<ProducableTile *> m_producables;

    ProducableGroupCollectionPtr m_producable_owners;
};

class StackableSubRegionGrid final {
public:
    explicit StackableSubRegionGrid(Grid<MapSubRegion> &&);
};

class TileSetIdGrid;

class TileSetMapElementVisitor;

class TileSetBase {
public:
    virtual ~TileSetBase() {}

    virtual void add_map_elements(TileSetMapElementVisitor &, const TileSetIdGrid & tid_layer) const = 0;
};

class TileSetMapElementVisitor {
public:
    virtual ~TileSetMapElementVisitor() {}

    virtual void add(StackableProducableTileGrid &&) = 0;

    virtual void add(StackableSubRegionGrid &&) = 0;
};

/// Tilesets map tileset ids to tile group fillers.
///
/// maybe a loader thing
class TileSet final {
public:
    using FillerFactory = SharedPtr<ProducableGroupFiller>(*)(const TileSetXmlGrid &, Platform &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;

    static const FillerFactoryMap & builtin_fillers();

    void load(Platform &, const TiXmlElement &,
              const FillerFactoryMap & = builtin_fillers());

    /// also clears out the entire tileset
    std::vector<SharedPtr<const ProducableGroupFiller>> move_out_fillers();

    SharedPtr<ProducableGroupFiller> find_filler(int tid) const;

    Vector2I tile_id_to_tileset_location(int tid) const;

    auto total_tile_count() const
        { return m_filler_grid.size(); }

    // should also pass along fillers...
    void add_map_elements(TileSetMapElementVisitor &, const TileSetIdGrid & tid_layer) const;

private:
    SharedPtr<ProducableGroupFiller> find_filler(Vector2I) const;

    Grid<SharedPtr<ProducableGroupFiller>> m_filler_grid;
    std::vector<SharedPtr<const ProducableGroupFiller>> m_unique_fillers;
};

class CompositeTileSet final {
public:
    // loading this: will have to go through steps of promises and so on
    // in order to load, so it needs to be non-blocking

    void load(Platform &, const TiXmlElement &);

    StackableSubRegionGrid something(const Grid<int> & tid_layer) const;

private:
    Grid<MapSubRegion> m_sub_regions_grid;
    const SharedPtr<MapRegion> & parent_region();
};
