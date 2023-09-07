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

#include "../Components.hpp"

class Platform;
class UnfinishedProducableTileViewGrid;
class TileMapIdToSetMapping;

class ProducableTileCallbacks {
public:
    virtual ~ProducableTileCallbacks() {}

    void add_collidable(const TriangleSegment & triangle)
        { add_collidable_(triangle); }

    void add_collidable(const Vector & triangle_point_a,
                        const Vector & triangle_point_b,
                        const Vector & triangle_point_c);

    template <typename ... Types>
    void add_entity(Types &&... arguments) {
        auto e = add_entity_();
        e.
            add<ModelScale, Types...>() =
            make_tuple(model_scale(), std::forward<Types>(arguments)...);
        if (e.has_all<ModelTranslation, ModelScale>()) {
            auto & trans = e.get<ModelTranslation>();
            auto & scale = e.get<ModelScale>();
            trans.value.x *= scale.value.x;
            trans.value.y *= scale.value.y;
            trans.value.z *= scale.value.z;
        }
    }

    virtual SharedPtr<RenderModel> make_render_model() = 0;

    // for later
    virtual void add_loader(const SharedPtr<LoaderTask> &) = 0;

protected:
    virtual void add_collidable_(const TriangleSegment &) = 0;

    virtual Entity add_entity_() = 0;

    virtual ModelScale model_scale() const = 0;
};

/// Represents how to make a single instance of a tile.
/// It is local to the entire in game field.
class ProducableTile {
public:
    virtual ~ProducableTile() {}

    /// @param maps_offset describes the position of the tile on the map itself
    // instead of maps_offset, can I pass TriangleSegmentTransformation instead?
    virtual void operator () (const Vector2I & maps_offset,
                              ProducableTileCallbacks &) const = 0;
};

using ProducableTileViewSubGrid = ViewGrid<ProducableTile *>::SubGrid;
class TileSet;
class ProducableGroupFiller;

/// A ViewGrid of producable tiles
///
/// An instance of this is used to represent a loaded map.
class ProducableTileViewGrid final {
public:
    using SubGrid = ViewGrid<ProducableTile *>::SubGrid;

    ProducableTileViewGrid() {}

    ProducableTileViewGrid
        (ViewGrid<ProducableTile *> && factory_view_grid,
         std::vector<SharedPtr<ProducableGroup_>> && groups,
         std::vector<SharedPtr<const ProducableGroupFiller>> && fillers);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    auto size2() const { return m_factories.size2(); }

    /** this object must live at least as long as the return value */
    auto make_subgrid(const RectangleI & range) const
        { return m_factories.make_subgrid(range); }

    auto make_subgrid() const { return m_factories.make_subgrid(); }

private:
    ViewGrid<ProducableTile *> m_factories;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
    // What was this meant to be used for?
    // So far, this object owns fillers and does nothing else with them.
    std::vector<SharedPtr<const ProducableGroupFiller>> m_fillers;
};

class UnfinishedProducableTileViewGrid final {
public:
    class ProducableGroupFillerExtraction {
    public:
        virtual ~ProducableGroupFillerExtraction()  {}

        virtual std::vector<SharedPtr<const ProducableGroupFiller>>
            move_out_fillers() = 0;
    };

    void add_layer
        (Grid<ProducableTile *> && target,
         const std::vector<SharedPtr<ProducableGroup_>> & groups);

    [[nodiscard]] ProducableTileViewGrid
        finish(ProducableGroupFillerExtraction &);

private:
    /// Moves out a view grid of all producables, and owners of those
    /// producables.
    ///
    /// As such they must be destroyed, and kept existing together.
    [[nodiscard]] Tuple
        <ViewGrid<ProducableTile *>,
         std::vector<SharedPtr<ProducableGroup_>>>
        move_out_producables_and_groups();

    std::vector<Grid<ProducableTile *>> m_targets;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

class ProducableGroupTileLayer final {
public:
    // may only be set once
    void set_size(const Size2I &);

    template <typename T>
    void add_group(UnfinishedProducableGroup<T> && unfinished_pgroup)
        { m_groups.emplace_back(unfinished_pgroup.finish(m_target)); }

    UnfinishedProducableTileViewGrid
        move_self_to(UnfinishedProducableTileViewGrid &&);

private:
    Grid<ProducableTile *> m_target;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};
