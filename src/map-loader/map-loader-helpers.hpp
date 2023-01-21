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
class GridView final {
public:
    using ElementContainer  = typename GridViewInserter<T>::ElementContainer;
    using ElementIterator   = typename ElementContainer::const_iterator;
    using ElementView       = View<ElementIterator>;
    using GridViewContainer = Grid<ElementView>;
    using Iterator          = typename GridViewContainer::Iterator;
    using ConstIterator     = typename GridViewContainer::ConstIterator;
    using SubGrid           = cul::ConstSubGrid
        <ElementView, cul::SubGridParentAccess::allow_access_to_parent_elements>;

    GridView() {}

    explicit GridView(GridViewInserter<T> && inserter) {
        verify_filled_inserter("GridView", inserter);
        std::tie(m_owning_container, m_views)
            = inserter.move_out_container_and_grid_view();
    }

    GridView(const GridView & rhs) {
        copy(rhs);
    }

    GridView(GridView && rhs):
        m_owning_container(std::move(rhs.m_owning_container)),
        m_views(std::move(rhs.m_views))
    {}

    GridView & operator = (const GridView & rhs) {
        if (this != &rhs) {
            copy(rhs);
        }
        return *this;
    }

    GridView & operator = (GridView && rhs) {
        m_owning_container = std::move(rhs.m_owning_container);
        m_views = std::move(rhs.m_views);
        return *this;
    }

    int width() const noexcept { return m_views.width(); }

    int height() const noexcept { return m_views.height(); }

    bool has_position(int x, int y) const noexcept { return m_views.has_position(x, y); }

    bool has_position(const Vector & r) const noexcept { return m_views.has_position(r); }

    Vector2I next(const Vector & r) const noexcept { return m_views.next(r); }

    Vector2I end_position() const noexcept { return m_views.end_position(); }

    Size2I size2() const noexcept { return m_views.size2(); }

    ElementView operator () (const Vector & r) const { return m_views(r); }

    ElementView operator () (int x, int y) const { return m_views(x, y); }

    Iterator begin() { return m_views.begin(); }
    Iterator end  () { return m_views.end  (); }

    ConstIterator begin() const { return m_views.begin(); }
    ConstIterator end  () const { return m_views.end  (); }

    std::size_t size() const noexcept { return m_views.size(); }

    void swap(GridView<T> & rhs) noexcept {
        m_owning_container.swap(rhs.m_owning_container);
        m_views.swap(rhs.m_views);
    }

    SubGrid make_subgrid(const RectangleI & rect) const {
        return SubGrid{m_views, cul::top_left_of(rect), rect.width, rect.height};
    }

private:
    void copy(const GridView & gridview) {
        // copying is much harder
        m_owning_container = gridview.m_owning_container;
        auto other_beg = gridview.m_owning_container.begin();
        m_views.clear();
        auto owning_beg = m_owning_container.begin();
        auto owning_end = m_owning_container.end();
        m_views.set_size(gridview.m_views.size2(), View{owning_end, owning_end});
        for (Vector2I r; r != gridview.end_position(); r = gridview.next(r)) {
            m_views = View{owning_beg + (gridview(r).beg() - other_beg),
                           owning_beg + (gridview(r).end() - other_beg)};
        }
    }

    void verify_filled_inserter
        (const char * caller, const GridViewInserter<T> & inserter)
    {
        using namespace cul::exceptions_abbr;
        if (inserter.filled()) return;
        throw InvArg{"GridView::" + std::string{caller} + ": "
                     "only accepts a filled grid view inserter"};
    }
    ElementContainer m_owning_container;
    GridViewContainer m_views;
};

// this will need to be moved too
class ProducableGroup_;

// ----------------------------------------------------------------------------

using ProducableTileViewSubGrid = GridView<ProducableTile *>::SubGrid;

class TileProducableViewGrid final {
public:
    // can reverse dependancy things later
    void set_layers(UnfinishedProducableTileGridView &&,
                    GidTidTranslator &&);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    /** this object must live at least as long as the return value */
    auto make_subgrid(const RectangleI & range) const
        { return m_factories.make_subgrid(range); }

private:
    GridView<ProducableTile *> m_factories;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
    std::vector<SharedPtr<const TileSet>> m_tilesets;
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
