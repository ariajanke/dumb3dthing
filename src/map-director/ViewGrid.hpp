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

#include "../Definitions.hpp"

#include <vector>

#include <ariajanke/cul/SubGrid.hpp>
#include <ariajanke/cul/RectangleUtils.hpp>

template <typename T>
class ViewGridInserterAttn;

template <typename T>
class ViewGrid;

template <typename T>
class ViewGridInserter final {
public:
    using ElementContainer = std::vector<T>;
    using ElementIterator  = typename std::vector<T>::const_iterator;
    using ElementView      = View<ElementIterator>;

    ViewGridInserter(int width, int height)
        { m_index_pairs.set_size(width, height, make_tuple(0, 0)); }

    explicit ViewGridInserter(const Size2I & size):
        ViewGridInserter(size.width, size.height) {}

    void advance();

    bool filled() const noexcept;

    ViewGrid<T> finish();

    void push(const T & obj);

    Vector2I position() const noexcept;

    template <typename U, typename Func>
    ViewGridInserter<U> transform_values(Func && f);

private:
    template <typename U>
    friend class ViewGridInserterAttn;

    ViewGridInserter(const Vector2I & position, std::vector<T> && elements,
                     Grid<Tuple<std::size_t, std::size_t>> && tuple_grid);

    Vector2I m_position;
    std::vector<T> m_elements;
    Grid<Tuple<std::size_t, std::size_t>> m_index_pairs;
};

// ----------------------------------------------------------------------------

template <typename T>
class ViewGrid final {
public:
    using ElementContainer  = typename ViewGridInserter<T>::ElementContainer;
    using ElementIterator   = typename ElementContainer::const_iterator;
    using ElementView       = View<ElementIterator>;
    using GridViewContainer = Grid<ElementView>;
    using Iterator          = typename GridViewContainer::Iterator;
    using ConstIterator     = typename GridViewContainer::ConstIterator;
    using SubGrid           = cul::ConstSubGrid
        <ElementView, cul::SubGridParentAccess::allow_access_to_parent_elements>;
    using Inserter          = ViewGridInserter<T>;

    ViewGrid() {}

    ViewGrid(ElementContainer &&, Grid<ElementView> &&);

    ViewGrid(const ViewGrid & rhs) { copy(rhs); }

    ViewGrid(ViewGrid &&);

    ViewGrid & operator = (const ViewGrid &);

    ViewGrid & operator = (ViewGrid &&);

    ElementView operator () (const Vector2I & r) const { return m_views(r); }

    ElementView operator () (int x, int y) const { return m_views(x, y); }

    Iterator begin() { return m_views.begin(); }

    ConstIterator begin() const { return m_views.begin(); }

    Iterator end() { return m_views.end  (); }

    ConstIterator end() const { return m_views.end  (); }

    Vector2I end_position() const noexcept { return m_views.end_position(); }

    bool has_position(int x, int y) const noexcept
        { return m_views.has_position(x, y); }

    bool has_position(const Vector2I & r) const noexcept
        { return m_views.has_position(r); }

    int height() const noexcept { return m_views.height(); }

    int width() const noexcept { return m_views.width(); }

    SubGrid make_subgrid(const RectangleI &) const;

    Vector2I next(const Vector2I & r) const noexcept { return m_views.next(r); }

    std::size_t size() const noexcept { return m_views.size(); }

    Size2I size2() const noexcept { return m_views.size2(); }

    void swap(ViewGrid<T> &) noexcept;

    auto elements_count() const { return m_owning_container.size(); }

    ElementView elements() const
        { return ElementView{m_owning_container.begin(), m_owning_container.end()}; }

private:
    void copy(const ViewGrid &);

    void verify_filled_inserter
        (const char * caller, const ViewGridInserter<T> &);

    ElementContainer m_owning_container;
    GridViewContainer m_views;
};

// ----------------------------------------------------------------------------

template <typename T>
class ViewGridInserterAttn final {
    template <typename U>
    friend class ViewGridInserter;

    static ViewGridInserter<T> make_new
        (const Vector2I & position, std::vector<T> && elements,
         Grid<Tuple<std::size_t, std::size_t>> && tuple_grid)
    {
        return ViewGridInserter<T>
            {position, std::move(elements), std::move(tuple_grid)};
    }
};

// ----------------------------------------------------------------------------

template <typename T>
void ViewGridInserter<T>::advance() {
    using namespace cul::exceptions_abbr;
    if (filled()) {
        throw RtError{"ViewGridInserter::advance: cannot advance a filled "
                      "inserter"};
    }
    auto el_count = m_elements.size();
    std::get<1>(m_index_pairs(m_position)) = el_count;
    auto next_position = m_index_pairs.next(m_position);
    if (next_position != m_index_pairs.end_position())
        m_index_pairs(next_position) = make_tuple(el_count, el_count);
    m_position = next_position;
}

template <typename T>
bool ViewGridInserter<T>::filled() const noexcept
    { return m_position == m_index_pairs.end_position(); }

template <typename T>
ViewGrid<T> ViewGridInserter<T>::finish() {
    Grid<View<ElementIterator>> views;
    views.set_size
        (m_index_pairs.width(), m_index_pairs.height(),
         ElementView{m_elements.end(), m_elements.end()});
    for (Vector2I r; r != m_index_pairs.end_position(); r = m_index_pairs.next(r)) {
        auto [beg_idx, end_idx] = m_index_pairs(r);
        views(r) = ElementView
            {m_elements.begin() + beg_idx, m_elements.begin() + end_idx};
    }
    return ViewGrid<T>{std::move(m_elements), std::move(views)};
}

template <typename T>
void ViewGridInserter<T>::push(const T & obj)
    { m_elements.push_back(obj); }

template <typename T>
Vector2I ViewGridInserter<T>::position() const noexcept
    { return m_position; }

template <typename T>
template <typename U, typename Func>
ViewGridInserter<U> ViewGridInserter<T>::transform_values(Func && f) {
    std::vector<U> new_values;
    new_values.reserve(m_elements.size());
    std::transform
        (m_elements.begin(), m_elements.end(),
         std::back_inserter(new_values), std::move(f));

    auto rv = ViewGridInserterAttn<U>::make_new
        (m_position, std::move(new_values), std::move(m_index_pairs));
    // reset this instance
    m_elements.clear();
    m_index_pairs.clear();
    m_position = Vector2I{};

    return rv;
}

template <typename T>
/* private */ ViewGridInserter<T>::ViewGridInserter
    (const Vector2I & position, std::vector<T> && elements,
     Grid<Tuple<std::size_t, std::size_t>> && tuple_grid):
    m_position   (position),
    m_elements   (std::move(elements  )),
    m_index_pairs(std::move(tuple_grid)) {}

// ----------------------------------------------------------------------------

template <typename T>
ViewGrid<T>::ViewGrid(ElementContainer && owning_container,
                      Grid<ElementView> && views):
    m_owning_container(std::move(owning_container)),
    m_views(std::move(views))
{}

template <typename T>
ViewGrid<T>::ViewGrid(ViewGrid && rhs):
    m_owning_container(std::move(rhs.m_owning_container)),
    m_views(std::move(rhs.m_views))
{}

template <typename T>
ViewGrid<T> & ViewGrid<T>::operator = (const ViewGrid & rhs) {
    if (this != &rhs) {
        copy(rhs);
    }
    return *this;
}

template <typename T>
ViewGrid<T> & ViewGrid<T>::operator = (ViewGrid && rhs) {
    m_owning_container = std::move(rhs.m_owning_container);
    m_views = std::move(rhs.m_views);
    return *this;
}

template <typename T>
typename ViewGrid<T>::SubGrid ViewGrid<T>::make_subgrid
    (const RectangleI & rect) const
{
    return SubGrid{m_views, cul::top_left_of(rect), rect.width, rect.height};
}

template <typename T>
void ViewGrid<T>::swap(ViewGrid<T> & rhs) noexcept {
    m_owning_container.swap(rhs.m_owning_container);
    m_views.swap(rhs.m_views);
}

template <typename T>
/* private */ void ViewGrid<T>::copy(const ViewGrid & gridview) {
    // copying is much harder
    m_owning_container = gridview.m_owning_container;
    auto other_beg = gridview.m_owning_container.begin();
    m_views.clear();
    auto owning_beg = m_owning_container.begin();
    auto owning_end = m_owning_container.end();
    ElementView empty_view{owning_end, owning_end};
    m_views.set_size(gridview.m_views.size2(), empty_view);
    for (Vector2I r; r != gridview.end_position(); r = gridview.next(r)) {
        auto beg_offset = gridview(r).begin() - other_beg;
        auto end_offset = gridview(r).end  () - other_beg;
        m_views(r) = ElementView{owning_beg + beg_offset, owning_beg + end_offset};
    }
}

template <typename T>
/* private */ void ViewGrid<T>::verify_filled_inserter
    (const char * caller, const ViewGridInserter<T> & inserter)
{
    using namespace cul::exceptions_abbr;
    if (inserter.filled()) return;
    throw InvArg{"GridView::" + std::string{caller} + ": "
                 "only accepts a filled grid view inserter"};
}
