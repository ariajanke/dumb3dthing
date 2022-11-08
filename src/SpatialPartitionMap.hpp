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

    ProjectionLine(const Vector & a_, const Vector & b_):
        m_a(a_), m_b(b_) {}

    Interval interval_for(const Triangle & triangle) const {
        std::array pts
            { triangle.point_a(), triangle.point_b(), triangle.point_c() };
        return interval_for(&pts[0], &pts[0] + pts.size());
    }

    Interval interval_for(const Vector & a, const Vector & b) const {
        std::array pts{a, b};
        return interval_for(&pts[0], &pts[0] + pts.size());
    }

    Real point_for(const Vector & r) const {
        auto pt_on_line = find_closest_point_to_line(m_a, m_b, r);
        auto mag = magnitude(pt_on_line - m_a);
        auto dir = dot(pt_on_line - m_a, m_b - m_a) < 0 ? -1 : 1;
        return mag*dir;
    }

private:    
    Interval interval_for(const Vector * beg, const Vector * end) const {
        assert(beg != end);
        auto [min, max] = std::minmax_element
            (beg, end,
             [this](const Vector & lhs, const Vector & rhs)
             { return point_for(lhs) < point_for(rhs); });
        return Interval{point_for(*min), point_for(*max)};
    }

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

    /** @brief Froms a pair based on the given interval.
     *
     *  @return returns a pair, the first element
     */
    Tuple<T, T> pair_for(const Interval & interval) const {
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
        ;
        return make_tuple(std::get<0>(*low), std::get<1>(*high));
    }

    void push(Real interval_end, const T & first, const T & second)
        { m_container.emplace_back(first, second, interval_end); }

    // a public verify feels like a smell to me
    void verify_sorted(const char * caller) const {
        using namespace cul::exceptions_abbr;
        if (is_sorted()) return;
        throw RtError{std::string{caller} + ": divisions must be sorted"};
    }

    void clear()
        { m_container.clear(); }

    auto count() const { return m_container.size(); }

    auto begin() const { return m_container.begin(); }

    auto end() const { return m_container.end(); }

private:
    using DivisionTuple = Tuple<T, T, Real>;
    using Container = std::vector<DivisionTuple>;

    static constexpr const auto k_div_element = 2;

    static bool ordered_tuples
        (const DivisionTuple & lhs, const DivisionTuple & rhs)
        { return std::get<k_div_element>(lhs) < std::get<k_div_element>(rhs); }

    template <typename Func>
    typename Container::const_iterator
        lower_bound(Real value, Func && pred) const
    {
        return std::lower_bound
            (m_container.begin(), m_container.end(), value, std::move(pred));
    }

    bool is_sorted() const {
        return std::is_sorted
            (m_container.begin(), m_container.end(), ordered_tuples);
    }

    std::vector<DivisionTuple> m_container;
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
        if (std::is_sorted
                (sorted_entries.begin(), sorted_entries.end(), compare_entries))
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

class ProjectedSpatialMap final {
public:
    using TriangleLinks = std::vector<SharedPtr<const TriangleLink>>;
    using Iterator = SpatialPartitionMap::Iterator;

    void populate(const TriangleLinks & links) {
        using EntryContainer = SpatialPartitionMap::EntryContainer;

        m_projection_line = make_line_for(links);

        EntryContainer entries;
        entries.reserve(links.size());
        for (auto & link : links) {
            entries.emplace_back
                (m_projection_line.interval_for(link->segment()), link);
        }
        std::sort
            (entries.begin(), entries.end(),
             SpatialPartitionMap::compare_entries);
        m_spatial_map.populate(entries);
    }

    cul::View<Iterator> view_for(const Vector & a, const Vector & b) const {
        return m_spatial_map.view_for
            ( m_projection_line.interval_for(a, b) );
    }

private:
    static ProjectionLine make_line_for(const TriangleLinks & links) {
        Vector low { k_inf,  k_inf,  k_inf};
        Vector high{-k_inf, -k_inf, -k_inf};
        for (auto & link : links) {
            const auto & triangle = link->segment();
            for (auto pt : { triangle.point_a(), triangle.point_b(), triangle.point_c() }) {
                auto low_list = {
                    make_tuple(&pt.x, &low.x),
                    make_tuple(&pt.y, &low.y),
                    make_tuple(&pt.z, &low.z),
                };
                auto high_list = {
                    make_tuple(&pt.x, &high.x),
                    make_tuple(&pt.y, &high.y),
                    make_tuple(&pt.z, &high.z),
                };
                for (auto [i, low_i] : low_list) {
                    *low_i = std::min(*low_i, *i);
                }
                for (auto [i, high_i] : high_list) {
                    *high_i = std::max(*high_i, *i);
                }
            }
        }
        auto line_options = {
            make_tuple(high.x - low.x, Vector{high.x, 0, 0}, Vector{low.x, 0, 0}),
            make_tuple(high.y - low.y, Vector{0, high.y, 0}, Vector{0, low.y, 0}),
            make_tuple(high.z - low.z, Vector{0, 0, high.z}, Vector{0, 0, low.z})
        };
        auto choice = std::max_element(line_options.begin(), line_options.end(),
                         [] (auto & lhs, auto & rhs)
        { return std::get<0>(lhs) < std::get<0>(rhs); });
        return ProjectionLine{std::get<1>(*choice), std::get<1>(*choice)};
    }

    SpatialPartitionMap m_spatial_map;
    ProjectionLine m_projection_line;
};
