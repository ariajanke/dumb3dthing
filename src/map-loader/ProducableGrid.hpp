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

#include "ProducableGroup.hpp"
#include "ViewGrid.hpp"

class EntityAndTrianglesAdder;
class Platform;
class UnfinishedProducableTileViewGrid;
class GidTidTranslator;

/// Represents how to make a single instance of a tile.
class ProducableTile {
public:
    virtual ~ProducableTile() {}

    virtual void operator () (const Vector2I & maps_offset,
                              EntityAndTrianglesAdder &, Platform &) const = 0;
};

using ProducableTileViewSubGrid = ViewGrid<ProducableTile *>::SubGrid;

/// A ViewGrid of producable tiles
///
/// An instance of this is used to represent a loaded map.
class ProducableTileViewGrid final {
public:
    // can reverse dependancy things later
    void set_layers(UnfinishedProducableTileViewGrid &&,
                    GidTidTranslator &&);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    /** this object must live at least as long as the return value */
    auto make_subgrid(const RectangleI & range) const
        { return m_factories.make_subgrid(range); }

private:
    ViewGrid<ProducableTile *> m_factories;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
    std::vector<SharedPtr<const TileSet>> m_tilesets;
};

// these can go into loading

class UnfinishedProducableTileViewGrid final {
public:
    void add_layer
        (Grid<ProducableTile *> && target,
         const std::vector<SharedPtr<ProducableGroup_>> & groups);

    /// Moves out a view grid of all producables, and owners of those
    /// producables.
    ///
    /// As such they must be destroyed, and kept existing together.
    [[nodiscard]] Tuple
        <ViewGrid<ProducableTile *>,
         std::vector<SharedPtr<ProducableGroup_>>>
        move_out_producables_and_groups();

private:
    std::vector<Grid<ProducableTile *>> m_targets;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

class UnfinishedTileGroupGrid final {
public:

    // may only be set once
    void set_size(const Size2I & sz) {
        m_target.set_size(sz, nullptr);
        m_filleds.set_size(sz, false);
    }

    template <typename T>
    void add_group(UnfinishedProducableGroup<T> && unfinished_pgroup) {
        m_groups.emplace_back(unfinished_pgroup.finish(m_target));
    }

    UnfinishedProducableTileViewGrid
        finish(UnfinishedProducableTileViewGrid && unfinished_grid_view)
    {
        m_filleds.clear();
        unfinished_grid_view.add_layer(std::move(m_target), m_groups);
        m_target.clear();
        m_groups.clear();
        return std::move(unfinished_grid_view);
    }

private:
    Grid<ProducableTile *> m_target;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
    // catch myself if I set a producable twice
    Grid<bool> m_filleds;
};
