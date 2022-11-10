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

class SpatialDivisionPairsBase {
protected:
    SpatialDivisionPairsBase() {}

    template <typename T>
    using DivisionTuple_ = Tuple<T, T, Real>;

    template <typename T>
    using Container_ = std::vector<DivisionTuple_<T>>;

    static constexpr const auto k_div_element = 2;

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
class SpatialDivisionPairsPopulator final : public SpatialDivisionPairsBase {
public:
    using Interval = ProjectionLine::Interval;
    using Container = Container_<T>;

    SpatialDivisionPairsPopulator() {}

    explicit SpatialDivisionPairsPopulator(Container && container_):
        m_container(std::move(container_)) {}

    void push(Real interval_end, const T & first, const T & second)
        { m_container.emplace_back(first, second, interval_end); }

    Container give_container()
        { return std::move(m_container); }

private:
    Container m_container;
};

template <typename T>
class SpatialDivisionPairs final : public SpatialDivisionPairsBase {
public:
    using Interval = ProjectionLine::Interval;
    using Populator = SpatialDivisionPairsPopulator<T>;

    SpatialDivisionPairs() {}

    explicit SpatialDivisionPairs(Populator &&);

    template <typename U, typename UtoT>
    SpatialDivisionPairs
        (const SpatialDivisionPairs<U> & other_divs, UtoT && u_to_t);

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
    using DivisionTuple = SpatialDivisionPairsBase::DivisionTuple_<T>;
    using Container = SpatialDivisionPairsBase::Container_<T>;

    template <typename Func>
    typename Container::const_iterator
        lower_bound(Real value, Func && pred) const;

    Container m_container;
};

template <typename Element>
class SpatialPartitionMapHelpers final {
public:
    using Interval = ProjectionLine::Interval;
    struct Entry final {
        Entry() {}
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
    using Divisions = SpatialDivisionPairs<T>;

    template <typename T>
    using DivisionsPopulator = SpatialDivisionPairsPopulator<T>;

    static std::vector<Real> compute_divisions
        (const EntryContainer &)
    {
        // a little more difficult, I'll do hard coded quarters to start
        return { 0.25, 0.5, 0.75, 1. };
    }

    static void make_indexed_divisions
        (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
         DivisionsPopulator<std::size_t> & index_divisions,
         EntryContainer & product_container);

    static EntryView
        view_for_entries
        (EntryIterator beg, EntryIterator end, Real start, Real last);

    static EntryIterator
        begin_for_entries(EntryIterator beg, EntryIterator end, Real start);

    static EntryIterator
        end_for_entries(EntryIterator beg, EntryIterator end, Real last);

private:
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
        explicit Iterator(EntryIterator itr_):
            m_itr(itr_) {}

        const Element & operator * () const { return m_itr->element; }

        Iterator & operator ++ ();

        bool operator != (const Iterator & rhs) const
            { return m_itr != rhs.m_itr; }

    private:
        EntryIterator m_itr;
    };

    static bool compare_entries(const Entry & lhs, const Entry & rhs)
        { return lhs.interval.min < rhs.interval.min; }

    void populate(const EntryContainer & sorted_entries);

    cul::View<Iterator> view_for(const Interval &) const;

private:
    template <typename T>
    using Divisions = SpatialDivisionPairs<T>;

    template <typename T>
    using DivisionsPopulator = SpatialDivisionPairsPopulator<T>;

    EntryContainer m_container;
    Divisions<EntryIterator> m_divisions;
};

class ProjectedSpatialMap final {
public:
    using TriangleLinks = std::vector<SharedPtr<const TriangleLink>>;
    using Iterator = SpatialPartitionMap::Iterator;

    void populate(const TriangleLinks &);

    cul::View<Iterator> view_for(const Vector &, const Vector &) const;

private:
    static ProjectionLine make_line_for(const TriangleLinks &);

    SpatialPartitionMap m_spatial_map;
    ProjectionLine m_projection_line;
};

// ----------------------------------------------------------------------------

template <typename T>
SpatialDivisionPairs<T>::SpatialDivisionPairs(Populator && population_):
    m_container(population_.give_container())
{
    using namespace cul::exceptions_abbr;
    if (is_sorted(m_container)) return;
    throw RtError{"SpatialDivisionPairs::SpatialDivisionPairs: divisions "
                  "must be sorted"};
}

template <typename T>
template <typename U, typename UtoT>
SpatialDivisionPairs<T>::SpatialDivisionPairs
    (const SpatialDivisionPairs<U> & other_divs, UtoT && u_to_t)
{
    m_container.reserve(other_divs.count());
    for (auto & [u0, u1, div] : other_divs) {
        m_container.emplace_back(u_to_t(u0), u_to_t(u1), div);
    }
}

template <typename T>
Tuple<T, T> SpatialDivisionPairs<T>::pair_for(const Interval & interval) const {
    // lower_bound for min, np
    auto low = lower_bound
        (interval.min,
         [] (const DivisionTuple & tup, Real min)
         { return std::get<k_div_element>(tup) < min; });
    // high end's a little tougher
    auto high = lower_bound
        (interval.max,
         [] (const DivisionTuple & tup, Real max)
         { return std::get<k_div_element>(tup) < max; });
    high = high == m_container.end() ? high - 1 : high;
    return make_tuple(std::get<0>(*low), std::get<1>(*high));
}

template <typename T>
SpatialDivisionPairsPopulator<T> SpatialDivisionPairs<T>::make_populator() {
    m_container.clear();
    return Populator{std::move(m_container)};
}

template <typename T>
template <typename Func>
/* private */ typename SpatialDivisionPairs<T>::Container::const_iterator
    SpatialDivisionPairs<T>::lower_bound(Real value, Func && pred) const
{
    return std::lower_bound
        (m_container.begin(), m_container.end(), value, std::move(pred));
}

// ----------------------------------------------------------------------------

template <typename Element>
/* static */ void SpatialPartitionMapHelpers<Element>::make_indexed_divisions
    (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
     DivisionsPopulator<std::size_t> & index_divisions,
     EntryContainer & product_container)
{
    Real last_division = 0;
    for (auto division : divisions) {
       const auto entries = view_for_entries
           (sorted_entries.begin(), sorted_entries.end(),
            last_division, division);
       auto old_size = product_container.size();
       product_container.insert
           (product_container.begin(), entries.begin(), entries.end());
       index_divisions.push
           (division, old_size, product_container.size());
       last_division = division;
    }
}

template <typename Element>
/* static */ typename SpatialPartitionMapHelpers<Element>::EntryView
    SpatialPartitionMapHelpers<Element>::view_for_entries
    (EntryIterator beg, EntryIterator end, Real start, Real last)
{
    // find the first entry that contains start
    // find the last entry that contains end (+1)
    beg = begin_for_entries(beg, end, start);
    return EntryView{beg, end_for_entries(beg, end, last)};
}

template <typename Element>
/* static */ typename SpatialPartitionMapHelpers<Element>::EntryIterator
    SpatialPartitionMapHelpers<Element>::begin_for_entries
    (EntryIterator beg, EntryIterator end, Real start)
{
    // returns first element that does not satisify element < value
    // I want the first element that contains start
    // therefore use a negative of that
    return std::lower_bound
        (beg, end, start,
         [](const Entry & element, Real start) {
            const auto & interval = element.interval;
            return interval.min < start;
         });
}

template <typename Element>
/* static */ typename SpatialPartitionMapHelpers<Element>::EntryIterator
    SpatialPartitionMapHelpers<Element>::end_for_entries
    (EntryIterator beg, EntryIterator end, Real last)
{
    // kinda like sweep I guess
    // linearly run from beg to end until last
    for (; beg != end; ++beg) {
        if (beg->interval.min > last)
            break;
    }
    return beg;
}
