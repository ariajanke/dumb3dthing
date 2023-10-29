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
#include "ScaleComputation.hpp"

#include "../Tasks.hpp"
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

    // for later
    virtual void add_loader(const SharedPtr<LoaderTask> &) = 0;

protected:
    virtual void add_collidable_(const TriangleSegment &) = 0;

    virtual Entity add_entity_() = 0;

    virtual ModelScale model_scale() const = 0;

    virtual ModelTranslation model_translation() const = 0;
};

// TODO deprecated/remove this class
class TriangleSegmentTransformation final {
public:
    TriangleSegmentTransformation() {}

    TriangleSegmentTransformation
        (const ScaleComputation & scale,
         const Vector2I & on_field_position):
        m_scale(scale),
        m_on_field_position(on_field_position) {}

    TriangleSegment operator () (const TriangleSegment & triangle) const;

    ModelTranslation model_translation() const;

    ModelScale model_scale() const;

private:
    Vector translation() const;

    ScaleComputation m_scale;
    Vector2I m_on_field_position;
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

class StackableProducableTileGrid;

class ProducableGroupTileLayer final {
public:
    static ProducableGroupTileLayer with_grid_size(const Size2I & sz)
        { return ProducableGroupTileLayer{sz}; }

    ProducableGroupTileLayer() {}

    template <typename T>
    void add_group(UnfinishedProducableGroup<T> && unfinished_pgroup)
        { m_groups.emplace_back(unfinished_pgroup.finish(m_target)); }

    StackableProducableTileGrid
        to_stackable_producable_tile_grid
        (const std::vector<SharedPtr<const ProducableGroupFiller>> & fillers);

private:
    explicit ProducableGroupTileLayer(const Size2I & target_size) {
        using namespace cul::exceptions_abbr;
        if (target_size.width <= 0 || target_size.height <= 0) {
            throw InvArg{"ProducableGroupTileLayer::set_size: given size must be "
                         "non zero"};
        }
        m_target.set_size(target_size, nullptr);
    }

    Grid<ProducableTile *> m_target;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};
