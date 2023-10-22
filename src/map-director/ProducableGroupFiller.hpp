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

    StackableProducableTileGrid() {}

    StackableProducableTileGrid
        (Grid<ProducableTile *> && producables,
         std::vector<SharedPtr<const ProducableGroupFiller>> && fillers,
         ProducableGroupCollection && producable_owners):
        m_producable_grids({ std::move(producables) }),
        m_fillers(std::move(fillers)),
        m_producable_owners(producable_owners) {}

    StackableProducableTileGrid stack_with(StackableProducableTileGrid &&);

    ProducableTileViewGrid to_producables();

private:
    std::vector<Grid<ProducableTile *>> m_producable_grids;
    std::vector<SharedPtr<const ProducableGroupFiller>> m_fillers;
    ProducableGroupCollection m_producable_owners;
};
