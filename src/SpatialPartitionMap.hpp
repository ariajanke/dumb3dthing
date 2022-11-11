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
    using DivisionTuple_ = Tuple<Real, T>;

    template <typename T>
    using Container_ = std::vector<DivisionTuple_<T>>;

    static constexpr const auto k_div_element = 0;

    template <typename T>
    static bool is_sorted(const Container_<T> & container) {
        return std::is_sorted
            (container.begin(), container.end(), compare_tuples<T>);
    }

private:
    template <typename T>
    static bool compare_tuples
        (const DivisionTuple_<T> & lhs, const DivisionTuple_<T> & rhs)
        { return std::get<k_div_element>(lhs) < std::get<k_div_element>(rhs); }
};

template <typename T>
class SpatialDivisionPopulator final : public SpatialDivisionBase {
public:
    using Interval = ProjectionLine::Interval;
    using Container = Container_<T>;

    SpatialDivisionPopulator() {}

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
    using DivisionTuple = SpatialDivisionBase::DivisionTuple_<T>;
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
    using EntryView = cul::View<EntryIterator>;

    template <typename T>
    using Divisions = SpatialDivisionContainer<T>;

    template <typename T>
    using DivisionsPopulator = SpatialDivisionPopulator<T>;

    static std::vector<Real> compute_divisions
        (const EntryContainer &)
    {
        // a little more difficult, I'll do hard coded quarters to start
        return { 0., 0.25, 0.5, 0.75, k_inf };
    }

    static void make_indexed_divisions
        (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
         DivisionsPopulator<std::size_t> & index_divisions,
         EntryContainer & product_container);

    static EntryView
        view_for_entries
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

    cul::View<Iterator> view_for(const Interval &) const;

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

    void populate(const TriangleLinks &);

    cul::View<Iterator> view_for(const Vector &, const Vector &) const;

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
Tuple<T, T> SpatialDivisionContainer<T>::pair_for(const Interval & interval) const {
    using std::get;
    using namespace cul::exceptions_abbr;
    // lower_bound for min, np
    auto low = lower_bound
        (interval.min,
         [] (const DivisionTuple & tup, Real min)
         { return get<k_div_element>(tup) < min; });

    // high end's a little tougher
    auto high = lower_bound
        (interval.max,
         [] (const DivisionTuple & tup, Real max)
         { return get<k_div_element>(tup) < max; });

    // it must be possible to return regardless of the given interval's values
    auto low_itr = get<1>(*( low  == m_container.begin() ? low : low - 1 ));
    auto end_itr = get<1>(*( high == m_container.end  () ? high - 1 : high ));

    return make_tuple(low_itr, end_itr);
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
    if (!is_sorted(m_container)) {
        throw RtError{"SpatialDivisionContainer::" + std::string{caller} +
                      ": divisions must be sorted"};
    }
    if (m_container.size() < 2) {
        throw RtError{"SpatialDivisionContainer::" + std::string{caller} +
                      ": container must have at least two elements"};
    }
    const auto make_no_inf_end_exp = [caller] {
        return RtError{"SpatialDivisionContainer::" + std::string{caller} +
                       ": divisions must end in an infinity"};
    };
#   if 0 // do I really need this?
    if (!m_container.empty()) {
        if (cul::is_real( std::get<k_div_element>(m_container.back()) )) {
            throw make_no_inf_end_exp();
        }
    }
#   endif
}

// ----------------------------------------------------------------------------

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
        if (last > interval.min && interval.max > start) {
            return itr;
        }
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
         [](Real last, const Entry & element) {
            const auto & interval = element.interval;
            return last < interval.min;
         });
}
