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

#include "TriangleLink.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

class ProjectionLine final {
public:
    using Triangle = TriangleSegment;
    struct Interval final {
        Interval() {}

        Interval(Real min_, Real max_):
            min(min_), max(max_) {}

        Real min = 0;
        Real max = 1;
    };

    ProjectionLine() {}

    ProjectionLine(const Vector &, const Vector &);

    Interval interval_for(const Triangle &) const;

    Interval interval_for(const Vector &, const Vector &) const;

    Real point_for(const Vector &) const;

private:
    Interval interval_for(const Vector * beg, const Vector * end) const;

    Vector m_a, m_b;
};

class SpatialDivisionBase {
protected:
    SpatialDivisionBase() {}

    template <typename T>
    struct Division_ final {
        Division_() {}

        Division_(Real pos_, T && el_):
            position(pos_), element(std::move(el_)) {}

        Division_(Real pos_, const T & el_):
            position(pos_), element(el_) {}

        Real position;
        T element;
    };

    template <typename T>
    using Container_ = std::vector<Division_<T>>;

    template <typename T>
    static bool is_sorted(const Container_<T> & container) {
        return std::is_sorted
            (container.begin(), container.end(), compare_tuples<T>);
    }

private:
    template <typename T>
    static bool compare_tuples
        (const Division_<T> & lhs, const Division_<T> & rhs)
        { return lhs.position < rhs.position; }
};

template <typename T>
class SpatialDivisionPopulator final : public SpatialDivisionBase {
public:
    using Interval = ProjectionLine::Interval;
    using Container = Container_<T>;

    SpatialDivisionPopulator() {}

    explicit SpatialDivisionPopulator(const std::vector<Tuple<Real, T>> & container_) {
        m_container.reserve(container_.size());
        for (auto [pos, el] : container_)
            { push(pos, el); }
    }

    explicit SpatialDivisionPopulator(Container && container_):
        m_container(std::move(container_)) {}

    void push(Real interval_start, const T & element)
        { m_container.emplace_back(interval_start, element); }

    Container && give_container()
        { return std::move(m_container); }

private:
    Container m_container;
};

/**
 *  Each division has a starting point, whose ending point is described by the
 *  next division's starting point.
 */
template <typename T>
class SpatialDivisionContainer final : public SpatialDivisionBase {
public:
    using Interval = ProjectionLine::Interval;
    using Populator = SpatialDivisionPopulator<T>;

    SpatialDivisionContainer() {}

    explicit SpatialDivisionContainer(Populator &&);

    template <typename U, typename UtoT>
    SpatialDivisionContainer
        (const SpatialDivisionContainer<U> & other_divs, UtoT && u_to_t);

    /** @brief Froms a pair based on the given interval.
     *
     *  @return returns a pair, the first element
     */
    Tuple<T, T> pair_for(const Interval &) const;

    Populator make_populator();

    auto count() const { return m_container.size(); }

    auto begin() const { return m_container.begin(); }

    auto end() const { return m_container.end(); }

private:
    using Division = SpatialDivisionBase::Division_<T>;
    using Container = SpatialDivisionBase::Container_<T>;

    template <typename Func>
    typename Container::const_iterator
        lower_bound(Real value, Func && pred) const;

    void verify_container(const char * caller) const;

    Container m_container;
};

template <typename Element>
class SpatialPartitionMapHelpers final {
public:
    using Interval = ProjectionLine::Interval;
    struct Entry final {
        Entry() {}
        Entry(Real min, Real max, const Element & el_):
            interval(min, max),
            element(el_) {}
        Entry(const Interval & intv_, const Element & el_):
            interval(intv_),
            element(el_) {}
        Interval interval;
        Element element;
    };

    using EntryContainer = std::vector<Entry>;
    using EntryIterator = typename EntryContainer::const_iterator;
    using EntryView = View<EntryIterator>;
    template <typename T>
    using Divisions = SpatialDivisionContainer<T>;
    template <typename T>
    using DivisionsPopulator = SpatialDivisionPopulator<T>;

    static std::vector<Real> compute_divisions
        (const EntryContainer & sorted_entries);

    static void make_indexed_divisions
        (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
         DivisionsPopulator<std::size_t> & index_divisions,
         EntryContainer & product_container);

    static EntryView view_for_entries
        (EntryIterator beg, EntryIterator end, Real start, Real last);

    static void sort_entries_container(EntryContainer & container)
        { std::sort(container.begin(), container.end(), compare_entries); }

    static bool is_sorted(const EntryContainer & container)
        { return std::is_sorted(container.begin(), container.end(), compare_entries); }

private:
    static EntryIterator
        begin_for_entries(EntryIterator beg, EntryIterator end, Real start, Real last);

    static EntryIterator
        end_for_entries(EntryIterator beg, EntryIterator end, Real last);

    static bool compare_entries(const Entry & lhs, const Entry & rhs)
        { return lhs.interval.min < rhs.interval.min; }

    SpatialPartitionMapHelpers() {}
};

class SpatialPartitionMap final {
public:
    using Interval = ProjectionLine::Interval;
    using Element = WeakPtr<const TriangleLink>;
    using Helpers = SpatialPartitionMapHelpers<Element>;
    using Entry = Helpers::Entry;
    using EntryContainer = Helpers::EntryContainer;
    using EntryIterator = Helpers::EntryIterator;
    using EntryView = Helpers::EntryView;

    class Iterator final {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = Element;
        using reference = const Element &;
        using pointer = const Element *;

        explicit Iterator(EntryIterator itr_):
            m_itr(itr_) {}

        const Element & operator * () const { return m_itr->element; }

        const Element * operator -> () const { return &m_itr->element; }

        Iterator & operator ++ ();

        bool operator != (const Iterator & rhs) const
            { return m_itr != rhs.m_itr; }

        bool operator == (const Iterator & rhs) const
            { return m_itr == rhs.m_itr; }

    private:
        EntryIterator m_itr;
    };

    SpatialPartitionMap() {}

    explicit SpatialPartitionMap(const EntryContainer & sorted_entries);

    void populate(const EntryContainer & sorted_entries);

    View<Iterator> view_for(const Interval &) const;

private:
    template <typename T>
    using Divisions = SpatialDivisionContainer<T>;

    template <typename T>
    using DivisionsPopulator = SpatialDivisionPopulator<T>;

    EntryContainer m_container;
    Divisions<EntryIterator> m_divisions;
};

class ProjectedSpatialMap final {
public:
    using TriangleLinks = std::vector<SharedPtr<const TriangleLink>>;
    using Iterator = SpatialPartitionMap::Iterator;
    using Helpers = SpatialPartitionMap::Helpers;

    ProjectedSpatialMap() {}

    explicit ProjectedSpatialMap(const TriangleLinks &);

    void populate(const TriangleLinks &);

    View<Iterator> view_for(const Vector &, const Vector &) const;

private:
    static ProjectionLine make_line_for(const TriangleLinks &);

    SpatialPartitionMap m_spatial_map;
    ProjectionLine m_projection_line;
};

// ----------------------------------------------------------------------------

template <typename T>
SpatialDivisionContainer<T>::SpatialDivisionContainer(Populator && population_):
    m_container(std::move(population_.give_container()))
{ verify_container("SpatialDivisionPairs"); }

template <typename T>
template <typename U, typename UtoT>
SpatialDivisionContainer<T>::SpatialDivisionContainer
    (const SpatialDivisionContainer<U> & other_divs, UtoT && u_to_t)
{
    m_container.reserve(other_divs.count());
    for (auto & [div, u] : other_divs) {
        m_container.emplace_back(div, u_to_t(u));
    }
    verify_container("SpatialDivisionContainer");
}

template <typename T>
Tuple<T, T> SpatialDivisionContainer<T>::pair_for
    (const Interval & interval) const
{
    auto tuple_lt_value = [] (const Division & div, Real value)
        { return div.position < value; };

    auto low  = lower_bound(interval.min, tuple_lt_value);
    auto high = lower_bound(interval.max, tuple_lt_value);

    // it must be possible to return regardless of the given interval's values
    low = low == m_container.begin() ? low : low - 1;

    // it must be the case that not interval over takes the last value
    assert(high != m_container.end());

    return make_tuple(low->element, high->element);
}

template <typename T>
SpatialDivisionPopulator<T> SpatialDivisionContainer<T>::make_populator() {
    m_container.clear();
    return Populator{std::move(m_container)};
}

template <typename T>
template <typename Func>
/* private */ typename SpatialDivisionContainer<T>::Container::const_iterator
    SpatialDivisionContainer<T>::lower_bound(Real value, Func && pred) const
{
    return std::lower_bound
        (m_container.begin(), m_container.end(), value, std::move(pred));
}

template <typename T>
/* private */ void SpatialDivisionContainer<T>::verify_container
    (const char * caller) const
{
    using namespace cul::exceptions_abbr;
    static auto make_head = [] (const char * caller)
        { return "SpatialDivisionContainer::" + std::string{caller}; };
    if (!is_sorted(m_container))
        { throw InvArg{make_head(caller) + ": divisions must be sorted"}; }
    if (m_container.size() < 2) {
        throw InvArg{  make_head(caller)
                     + ": container must have at least two elements"};
    }
    // this class is built on lower bound, there must be an element that no
    // interval/position comes after, and therefore, there must be a last
    // "infinity" element
    if (m_container.back().position != k_inf) {
        throw InvArg{  make_head(caller)
                     + ": last element must be at infinity, as it must be the "
                       "case that no interval position overtakes the last "
                       "element in the container"};
    }
}

// ----------------------------------------------------------------------------

template <typename Element>
/* static */ std::vector<Real>
    SpatialPartitionMapHelpers<Element>::compute_divisions
    (const EntryContainer & entries)
{
    using namespace cul::exceptions_abbr;
    if (entries.empty())
        { return { 0., k_inf }; }
    if (!is_sorted(entries)) {
        throw InvArg{"SpatialPartitionMapHelpers::compute_divisions:"
                     "entries must be sorted"};
    }
    // a little more difficult, I'll do hard coded quarters to start
    auto first_entry_max_is_min = [](const Entry & lhs, const Entry & rhs)
        { return lhs.interval.max < rhs.interval.max; };

    // they're sorted after all...
    auto min = entries.front().interval.min;
    auto max = std::max_element
        (entries.begin(), entries.end(), first_entry_max_is_min)
        ->interval.max;
    // v "denormalize" these divisions v
    std::vector<Real> divisions{ 0., 0.25, 0.5, 0.75, 0. };
    for (auto & div : divisions) {
        div = min + div*(max - min);
    }
    divisions.back() = k_inf;
    return divisions;
}

template <typename Element>
/* static */ void SpatialPartitionMapHelpers<Element>::make_indexed_divisions
    (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
     DivisionsPopulator<std::size_t> & index_divisions,
     EntryContainer & product_container)
{
    using namespace cul::exceptions_abbr;
    if (divisions.size() == 1) {
        throw InvArg{"SpatialPartitionMapHelpers::make_indexed_divisions: "
                     "divisions may not contain only one element"};
    }
    for (auto itr = divisions.begin(); itr + 1 != divisions.end(); ++itr) {
        auto low = *itr;
        auto high = *(itr + 1);
        const auto entries = view_for_entries
            (sorted_entries.begin(), sorted_entries.end(), low, high);
        index_divisions.push(low, product_container.size());
        product_container.insert
            (product_container.end(), entries.begin(), entries.end());
    }
    index_divisions.push(k_inf, product_container.size());
}

template <typename Element>
/* static */ typename SpatialPartitionMapHelpers<Element>::EntryView
    SpatialPartitionMapHelpers<Element>::view_for_entries
    (EntryIterator beg, EntryIterator end, Real start, Real last)
{
    // find the first entry that contains start
    // find the last entry that contains end (+1)
    end = end_for_entries(beg, end, last);
    return EntryView{begin_for_entries(beg, end, start, last), end};
}

template <typename Element>
/* private static */ typename SpatialPartitionMapHelpers<Element>::EntryIterator
    SpatialPartitionMapHelpers<Element>::begin_for_entries
    (EntryIterator beg, EntryIterator end, Real start, Real last)
{
    // how do I know I've hit that last overlapping the interval?
    // the only sure way to do it, is to do it linearly
    // (which case order of running does not matter)
    // there maybe an implementation in the future, where I can reduce it
    // (perhaps yet another trade memory in for speed)
    for (auto itr = beg; itr != end; ++itr) {
        const auto & interval = itr->interval;
        if (last > interval.min && interval.max > start)
            { return itr; }
    }
    return end;
}

template <typename Element>
/* private static */ typename SpatialPartitionMapHelpers<Element>::EntryIterator
    SpatialPartitionMapHelpers<Element>::end_for_entries
    (EntryIterator beg, EntryIterator end, Real last)
{
    // find the first position to "insert" last
    // the first min above last is the end iterator
    return std::upper_bound
        (beg, end, last,
         [](Real last, const Entry & element)
         { return last < element.interval.min; });
}
