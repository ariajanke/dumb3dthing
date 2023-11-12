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

#include "../Definitions.hpp"

#include <rigtorp/HashMap.h>

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_tileset;
};

/// How to fill out a grid with a tile group.
///
///
class ProducableGroupFiller {
public:
    virtual ~ProducableGroupFiller() {}

    virtual ProducableGroupTileLayer operator ()
        (const std::vector<TileLocation> &, ProducableGroupTileLayer &&) const = 0;
};

class StackableProducableTileGrid final {
public:
    using ProducableGroupCollection = std::vector<SharedPtr<ProducableGroup_>>;
    using ProducableGroupCollectionPtr = SharedPtr<ProducableGroupCollection>;
    using ProducableFillerMapHasher = std::hash<SharedPtr<const ProducableGroupFiller>>;
    using ProducableFillerMap =
        rigtorp::HashMap<SharedPtr<const ProducableGroupFiller>,
                         std::monostate,
                         ProducableFillerMapHasher>;

    static Optional<ViewGrid<ProducableTile *>>
        producable_grids_to_view_grid
        (std::vector<Grid<ProducableTile *>> && producables);

    static StackableProducableTileGrid make_with_fillers
        (const std::vector<SharedPtr<const ProducableGroupFiller>> &,
         Grid<ProducableTile *> && producables,
         ProducableGroupCollection && producable_owners);

    static std::vector<SharedPtr<const ProducableGroupFiller>>
        filler_map_to_vector(ProducableFillerMap &&);

    StackableProducableTileGrid();

    StackableProducableTileGrid
        (Grid<ProducableTile *> && producables,
         ProducableFillerMap && fillers,
         ProducableGroupCollection && producable_owners);

    StackableProducableTileGrid stack_with(StackableProducableTileGrid &&);

    StackableProducableTileGrid stack_with
        (std::vector<Grid<ProducableTile *>> && producable_grids,
         ProducableFillerMap && fillers,
         ProducableGroupCollection && producable_owners);

    ProducableTileViewGrid to_producables();

private:
    static const ProducableFillerMap k_default_producable_filler_map;

    StackableProducableTileGrid
        (std::vector<Grid<ProducableTile *>> && producable_grids,
         ProducableFillerMap && fillers,
         ProducableGroupCollection && producable_owners);

    std::vector<Grid<ProducableTile *>> m_producable_grids;
    ProducableFillerMap m_fillers;
    ProducableGroupCollection m_producable_owners;
};
