/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "../Components.hpp"
#include "TileFactory.hpp"
#include "TileSet.hpp"

#include <ariajanke/cul/SubGrid.hpp>

class TeardownTask final : public LoaderTask {
public:
    TeardownTask() {}

    explicit TeardownTask
        (std::vector<Entity> && entities,
         std::vector<SharedPtr<TriangleLink>> && triangles):
        m_entities (std::move(entities )),
        m_triangles(std::move(triangles)) {}

    void operator () (Callbacks &) const final;

private:
    std::vector<Entity> m_entities;
    std::vector<SharedPtr<TriangleLink>> m_triangles;
};

using TileFactorySubGrid = cul::ConstSubGrid
    <TileFactory *,
     cul::SubGridParentAccess::allow_access_to_parent_elements>;

class TileFactoryGrid final {
public:
    using Rectangle = cul::Rectangle<int>;

    void load_layer(const Grid<int> & gids, const GidTidTranslator &);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    auto is_empty() const { return m_factories.is_empty(); }

    /** this object must live at least as long as the return value */
    TileFactorySubGrid make_subgrid(const Rectangle & range) const;

private:
    std::vector<SharedPtr<const TileSet>> m_tilesets;
    Grid<TileFactory *> m_factories;
};

/// container of triangle links, used to glue segment triangles together
class InterTriangleLinkContainer final {
public:
    using Iterator = std::vector<SharedPtr<TriangleLink>>::iterator;
    using GridOfViews = Grid<View<TriangleLinks::const_iterator>>;

    InterTriangleLinkContainer() {}

    explicit InterTriangleLinkContainer(const GridOfViews & views);

    void glue_to(InterTriangleLinkContainer & rhs);

private:
    static bool is_edge_tile(const GridOfViews & grid, const Vector2I & r);

    static bool is_not_edge_tile(const GridOfViews & grid, const Vector2I & r);

    template <bool (*meets_pred)(const GridOfViews &, const Vector2I &)>
    static void append_links_by_predicate
        (const GridOfViews & views, std::vector<SharedPtr<TriangleLink>> & links);

    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    std::vector<SharedPtr<TriangleLink>> m_links;
    Iterator m_edge_begin;
};

struct Vector2IHasher final {
    std::size_t operator () (const Vector2I & r) const {
        using IntHash = std::hash<int>;
        return IntHash{}(r.x) ^ IntHash{}(r.y);
    }
};

template <typename T>
class GridViewInserterAttn;

template <typename T>
class GridViewInserter final {
public:
    using ElementContainer = std::vector<T>;
    using ElementIterator  = typename std::vector<T>::const_iterator;
    using ElementView      = View<ElementIterator>;
    using Size2I           = cul::Size2<int>;

    GridViewInserter(int width, int height)
        { m_index_pairs.set_size(width, height, make_tuple(0, 0)); }

    explicit GridViewInserter(Size2I size):
        GridViewInserter(size.width, size.height) {}

    void advance();

    void push(const T & obj);

    Vector2I position() const noexcept;

    bool filled() const noexcept;

    Tuple<ElementContainer, Grid<ElementView>>
        move_out_container_and_grid_view();

    template <typename U, typename Func>
    GridViewInserter<U> transform_values(Func && f);

private:
    template <typename U>
    friend class GridViewInserterAttn;

    GridViewInserter(const Vector2I & position, std::vector<T> && elements,
                     Grid<Tuple<std::size_t, std::size_t>> && tuple_grid):
        m_position   (position),
        m_elements   (std::move(elements  )),
        m_index_pairs(std::move(tuple_grid)) {}

    Vector2I m_position;
    std::vector<T> m_elements;
    Grid<Tuple<std::size_t, std::size_t>> m_index_pairs;
};

template <typename T>
class GridViewInserterAttn final {
    template <typename U>
    friend class GridViewInserter;

    static GridViewInserter<T> make_new
        (const Vector2I & position, std::vector<T> && elements,
         Grid<Tuple<std::size_t, std::size_t>> && tuple_grid)
    {
        return GridViewInserter<T>
            {position, std::move(elements), std::move(tuple_grid)};
    }
};

// ----------------------------------------------------------------------------

template <typename T>
void GridViewInserter<T>::advance() {
    using namespace cul::exceptions_abbr;
    if (filled()) {
        throw RtError{""};
    }
    auto el_count = m_elements.size();
    std::get<1>(m_index_pairs(m_position)) = el_count;
    auto next_position = m_index_pairs.next(m_position);
    if (next_position != m_index_pairs.end_position())
        m_index_pairs(next_position) = make_tuple(el_count, el_count);
    m_position = next_position;
}

template <typename T>
void GridViewInserter<T>::push(const T & obj)
    { m_elements.push_back(obj); }

template <typename T>
Vector2I GridViewInserter<T>::position() const noexcept
    { return m_position; }

template <typename T>
bool GridViewInserter<T>::filled() const noexcept
    { return m_position == m_index_pairs.end_position(); }

template <typename T>
    Tuple<typename GridViewInserter<T>::ElementContainer,
          Grid<typename GridViewInserter<T>::ElementView>>
    GridViewInserter<T>::move_out_container_and_grid_view()
{
    Grid<View<ElementIterator>> views;
    views.set_size
        (m_index_pairs.width(), m_index_pairs.height(),
         ElementView{m_elements.end(), m_elements.end()});
    for (Vector2I r; r != m_index_pairs.end_position(); r = m_index_pairs.next(r)) {
        auto [beg_idx, end_idx] = m_index_pairs(r);
        views(r) = ElementView
            {m_elements.begin() + beg_idx, m_elements.begin() + end_idx};
    }
    return make_tuple(std::move(m_elements), std::move(views));
}

template <typename T>
template <typename U, typename Func>
GridViewInserter<U> GridViewInserter<T>::transform_values(Func && f) {
    std::vector<U> new_values;
    new_values.reserve(m_elements.size());
    std::transform
        (m_elements.begin(), m_elements.end(),
         std::back_inserter(new_values), std::move(f));

    auto rv = GridViewInserterAttn<U>::make_new
        (m_position, std::move(new_values), std::move(m_index_pairs));
    // reset this instance
    m_elements.clear();
    m_index_pairs.clear();
    m_position = Vector2I{};

    return rv;
}
