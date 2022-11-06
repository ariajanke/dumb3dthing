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

// fairly "static"
// dupelicates allowed in views

class ProjectionLine final {
public:
    using Triangle = TriangleSegment;
    struct Interval final {
        Real min = 0;
        Real max = 1;
    };

    ProjectionLine() {}

    ProjectionLine(const Vector & a_, const Vector & b_):
        m_a(a_), m_b(b_) {}

    Interval interval_for(const Triangle &) const;

    Real point_for(const Vector &) const;

private:
    Vector m_a, m_b;
};

template <typename T>
class SpatialDivisionPairs final {
public:
    using Interval = ProjectionLine::Interval;

    SpatialDivisionPairs() {}

    template <typename U, typename UtoT>
    SpatialDivisionPairs(const SpatialDivisionPairs<U> & other_divs, UtoT && u_to_t) {
        m_container.reserve(other_divs.count());
        for (auto & [u0, u1, div] : other_divs) {
            m_container.emplace_back(u_to_t(u0), u_to_t(u1), div);
        }
    }

    Tuple<T, T> pair_for(const Interval &) const;

    void push(Real interval_end, const T &, const T &);

    void verify_sorted(const char * caller) const;

    void clear()
        { m_container.clear(); }

    auto count() const { return m_container.size(); }

    auto begin() const { return m_container.begin(); }

    auto end() const { return m_container.end(); }

private:
    std::vector<Tuple<T, T, Real>> m_container;
};

class SpatialPartitionMap final {
public:
    using Interval = ProjectionLine::Interval;
    using Element = WeakPtr<const TriangleLink>;
    struct Entry final {
        Entry() {}
        Entry(const Interval & intv_, const Element & el_):
            interval(intv_),
            element(el_) {}
        Interval interval;
        Element element;
    };
    using EntryContainer = std::vector<Entry>;
    using EntryIterator = EntryContainer::const_iterator;

    class Iterator final {
    public:
        explicit Iterator(EntryIterator itr_):
            m_itr(itr_) {}

        const Element & operator * () const { return m_itr->element; }

        Iterator & operator ++ () {
            ++m_itr;
            return *this;
        }

        bool operator != (const Iterator & rhs) const
            { return m_itr != rhs.m_itr; }

    private:
        EntryIterator m_itr;
    };

    static bool compare_entries(const Entry & lhs, const Entry & rhs) {
        return lhs.interval.min < rhs.interval.min;
    }

    void populate(const EntryContainer & sorted_entries) {
        using namespace cul::exceptions_abbr;
        if (std::is_sorted(sorted_entries.begin(), sorted_entries.end(),
                compare_entries))
        { throw InvArg{"entries must be sorted"}; }

        m_divisions.clear();
        m_container.clear();

        // sorted_entries is our temporary
        auto divisions = compute_divisions(sorted_entries);

        // indicies represent would be positions in the destination container
        Divisions<std::size_t> index_divisions;
        make_indexed_divisions
            (sorted_entries, divisions, index_divisions, m_container);
        index_divisions.verify_sorted("populate");

        // after all entries are in, convert indicies into iterators
        m_divisions = Divisions<EntryIterator>
            {index_divisions,
             [this](std::size_t idx) { return m_container.begin() + idx; }};
    }

    cul::View<Iterator> view_for(const Interval & interval) const {
        auto [beg, end] = m_divisions.pair_for(interval); {}
        return cul::View{Iterator{beg}, Iterator{end}};
    }

private:
    using EntryView = cul::View<EntryIterator>;

    template <typename T>
    using Divisions = SpatialDivisionPairs<T>;

    static std::vector<Real> compute_divisions
        (const EntryContainer &)
    {
        // a little more difficult, I'll do hard coded quarters to start
        return { 0.25, 0.5, 0.75, 1. };
    }

    static void make_indexed_divisions
        (const EntryContainer & sorted_entries, const std::vector<Real> & divisions,
         Divisions<std::size_t> & index_divisions, EntryContainer & product_container)
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

    static EntryView
        view_for_entries
        (EntryIterator beg, EntryIterator end, Real start, Real last)
    {
        // find the first entry that contains start
        // find the last entry that contains end (+1)
        beg = begin_for_entries(beg, end, start);
        return EntryView{beg, end_for_entries(beg, end, last)};
    }

    static EntryIterator
        begin_for_entries(EntryIterator beg, EntryIterator end, Real start)
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

    static EntryIterator
        end_for_entries(EntryIterator beg, EntryIterator end, Real last)
    {
        // kinda like sweep I guess
        // linearly run from beg to end until last
        for (; beg != end; ++beg) {
            if (beg->interval.min > last)
                break;
        }
        return beg;
    }

    EntryContainer m_container;
    Divisions<EntryIterator> m_divisions;
};
