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

#include "ViewGrid.hpp"

#include "../Components.hpp"

class Platform;
class UnfinishedProducableTileViewGrid;

class ProducableTileCallbacks {
public:
    virtual ~ProducableTileCallbacks() {}

    void add_collidable(const TriangleSegment & triangle)
        { add_collidable_(triangle); }

    void add_collidable(const Vector & triangle_point_a,
                        const Vector & triangle_point_b,
                        const Vector & triangle_point_c);

    template <typename ... Types>
    Entity add_entity(Types &&... arguments) {
        auto e = add_entity_();
        e.
            add<ModelScale, ModelTranslation, Types...>() =
            make_tuple(model_scale(), model_translation(),
                       std::forward<Types>(arguments)...  );
        return e;
    }

    virtual SharedPtr<RenderModel> make_render_model() = 0;

protected:
    virtual void add_collidable_(const TriangleSegment &) = 0;

    virtual Entity add_entity_() = 0;

    virtual ModelScale model_scale() const = 0;

    virtual ModelTranslation model_translation() const = 0;
};

/// Represents how to make a single instance of a tile.
/// It is local to the entire in game field.
class ProducableTile {
public:
    virtual ~ProducableTile() {}

    /// @param maps_offset describes the position of the tile on the map itself
    // instead of maps_offset, can I pass TriangleSegmentTransformation instead?
    // Tuple<ModelTranslation, ModelScale>?
    virtual void operator () (ProducableTileCallbacks &) const = 0;
};

using ProducableTileViewSubGrid = ViewGrid<ProducableTile *>::SubGrid;
class ProducablesTileset;
class ProducableGroupFiller;

/// A producable group owns a set of producable tiles.
class ProducableGroupOwner {
public:
    virtual ~ProducableGroupOwner() {}
};

/// A ViewGrid of producable tiles
///
/// An instance of this is used to represent a loaded map.
class ProducableTileViewGrid final {
public:
    using SubGrid = ViewGrid<ProducableTile *>::SubGrid;

    ProducableTileViewGrid() {}

    ProducableTileViewGrid
        (ViewGrid<ProducableTile *> && factory_view_grid,
         std::vector<SharedPtr<ProducableGroupOwner>> && groups);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    auto size2() const { return m_factories.size2(); }

    /** this object must live at least as long as the return value */
    auto make_subgrid(const RectangleI & range) const
        { return m_factories.make_subgrid(range); }

    auto make_subgrid() const { return m_factories.make_subgrid(); }

private:
    ViewGrid<ProducableTile *> m_factories;
    std::vector<SharedPtr<ProducableGroupOwner>> m_groups;
};
