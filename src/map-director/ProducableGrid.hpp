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
class AssetsRetrieval;

template <typename ... Types>
class EntityTupleBuilder final {
public:
    EntityTupleBuilder() {}

    explicit EntityTupleBuilder
        (Entity &&, TupleBuilder<Types...> && = TupleBuilder<Types...>{});

    explicit EntityTupleBuilder(Tuple<Types...> && tuple);

    template <typename T>
    [[nodiscard]] EntityTupleBuilder<T, Types...> add(T && obj) &&;

    template <typename T>
    [[nodiscard]] T & get() { return m_builder.template get<T>(); }

    Entity finish() &&;

private:
    TupleBuilder<Types...> m_builder;
    Entity m_entity;
};

template <typename ... Types>
EntityTupleBuilder<Types...>::EntityTupleBuilder
    (Entity && entity,
     TupleBuilder<Types...> && builder):
    m_builder(std::move(builder)),
    m_entity(std::move(entity))
{
    assert(!entity);
    assert(m_entity);
}

template <typename ... Types>
template <typename T>
EntityTupleBuilder<T, Types...>
    EntityTupleBuilder<Types...>::add(T && obj) &&
{
    if (!m_entity) {
        throw RuntimeError
            {"Cannot reuse EntityTupleBuilder, instantiate a new one instead"};
    }
    auto next_builder = std::move(m_builder).template add<T>(std::move(obj));
    return EntityTupleBuilder<T, Types...>
        (std::move(m_entity), std::move(next_builder));
}

template <typename ... Types>
Entity EntityTupleBuilder<Types...>::finish() && {
    static_assert(sizeof...(Types) > 1, "must implement fewer than two types finisher");
    m_entity.add<Types...>() = std::move(m_builder).finish();
    return m_entity;
}

class ProducableTileCallbacks {
public:
    using StartingTupleBuilder = EntityTupleBuilder<ModelScale, ModelTranslation>;

    virtual ~ProducableTileCallbacks() {}

    void add_collidable(const TriangleSegment & triangle)
        { add_collidable_(triangle); }

    void add_collidable(const Vector & triangle_point_a,
                        const Vector & triangle_point_b,
                        const Vector & triangle_point_c);

    virtual StartingTupleBuilder add_entity() = 0;

    /// RNG is tile location dependant (no producable should need to know
    /// where exactly it is on the field)
    /// @returns Real number in range [-0.5 0.5]
    virtual Real next_random() = 0;

    virtual AssetsRetrieval & assets_retrieval() const = 0;

    virtual SharedPtr<RenderModel> make_render_model() = 0;

protected:
    virtual void add_collidable_(const TriangleSegment &) = 0;

#   if 0
    virtual Entity add_entity_() = 0;
#   endif
    virtual ModelScale model_scale() const = 0;

    virtual ModelTranslation model_translation() const = 0;

    StartingTupleBuilder add_default_entity(Entity && ent) const {
        return StartingTupleBuilder
            {std::move(ent),
             TupleBuilder{}.
                add(model_translation()).
                add(model_scale())};
    }
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

protected:
    ProducableGroupOwner() {}
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
