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

#include "RegionAxisLinksContainer.hpp"

#include "../Configuration.hpp"
#include "../TriangleLink.hpp"

#include <iostream>

namespace {

constexpr const bool k_report_maximum_sort_and_sweep =
    k_region_axis_container_report_maximum_sort_and_sweep;

Real get_x(const Vector & r) { return r.x; }

Real get_z(const Vector & r) { return r.z; }

template <typename ItemType, typename IteratorType, typename ComparisonFunc>
ItemType * generalize_seek
    (IteratorType begin, IteratorType end, ComparisonFunc &&);

} // end of <anonymous> namespace

/* static */ bool RegionAxisLinkEntry::bounds_less_than
    (const RegionAxisLinkEntry & lhs, const RegionAxisLinkEntry & rhs)
{ return lhs.low_bounds() < rhs.low_bounds(); }

/* static */ bool RegionAxisLinkEntry::pointer_less_than
    (const RegionAxisLinkEntry & lhs, const RegionAxisLinkEntry & rhs)
{ return lhs.link() < rhs.link(); }

/* static */ bool RegionAxisLinkEntry::pointer_equal
    (const RegionAxisLinkEntry & lhs, const RegionAxisLinkEntry & rhs)
{ return lhs.link() == rhs.link(); }

/* static */ bool RegionAxisLinkEntry::linkless(const RegionAxisLinkEntry & entry)
    { return !entry.link(); }

/* static */ void RegionAxisLinkEntry::attach_matching_points
    (const RegionAxisLinkEntry & lhs, const RegionAxisLinkEntry & rhs)
{ TriangleLink::attach_unattached_matching_points(lhs.link(), rhs.link()); }

/* static */ RegionAxisLinkEntry RegionAxisLinkEntry::computed_bounds
    (const SharedPtr<TriangleLink> & link_ptr, RegionAxis axis)
{
    using Axis = RegionAxis;
    switch (axis) {
    case Axis::x_ways: return computed_bounds_<get_x>(link_ptr);
    case Axis::z_ways: return computed_bounds_<get_z>(link_ptr);
    default          : break;
    }
    throw RuntimeError{":c"};
}

RegionAxisLinkEntry::RegionAxisLinkEntry
    (Real low_, Real high_, const SharedPtr<TriangleLink> & link_ptr):
    m_low(low_), m_high(high_), m_link_ptr(link_ptr) {}

template <Real (*get_i)(const Vector &)>
/* private static */ RegionAxisLinkEntry RegionAxisLinkEntry::computed_bounds_
    (const SharedPtr<TriangleLink> & link_ptr)
{
    Real low = k_inf;
    Real high = -k_inf;
    const auto & triangle = link_ptr->segment();
    for (auto pt : { triangle.point_a(), triangle.point_b(), triangle.point_c() }) {
        high = std::max(high, get_i(pt));
        low  = std::min(low , get_i(pt));
    }
    return RegionAxisLinkEntry{low, high, link_ptr};
}

// ----------------------------------------------------------------------------

RegionAxisLinksContainer::RegionAxisLinksContainer
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_entries(std::move(entries_)),
    m_axis(axis_) {}

RegionAxisLinksRemover RegionAxisLinksContainer::make_remover()
    { return RegionAxisLinksRemover{std::move(m_entries), m_axis}; }

RegionAxisLinksAdder RegionAxisLinksContainer::make_adder(RegionAxis axis_)
    { return RegionAxisLinksAdder{std::move(m_entries), axis_}; }

RegionAxisLinksAdder RegionAxisLinksContainer::make_adder()
    { return RegionAxisLinksAdder{std::move(m_entries), m_axis}; }

// ----------------------------------------------------------------------------

/* static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksAdder::dedupelicate
    (std::vector<RegionAxisLinkEntry> && entries)
{
    std::sort(entries.begin(), entries.end(),
              RegionAxisLinkEntry::pointer_less_than);
    auto uend = std::unique
        (entries.begin(), entries.end(), RegionAxisLinkEntry::pointer_equal);
    entries.erase(uend, entries.end());
    return std::move(entries);
}

/* static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksAdder::sort_and_sweep
    (std::vector<RegionAxisLinkEntry> && entries)
{
    static int s_sort_sweep_max = 0;
    int sort_sweep_count = 0;
    std::sort(entries.begin(), entries.end(),
              RegionAxisLinkEntry::bounds_less_than);
#   if 0//def MACRO_DEBUG
    assert(std::all_of(entries.begin(), entries.end(),
        [] (const RegionAxisLinkEntry & entry) {
            return !are_very_close(entry.low_bounds(), entry.high_bounds());
        }));
#   endif
    for (auto itr = entries.begin(); itr != entries.end(); ++itr) {
        for (auto jtr = itr + 1; jtr != entries.end(); ++jtr) {
            if (itr->high_bounds() < jtr->low_bounds())
                break;
            if constexpr (k_report_maximum_sort_and_sweep)
                ++sort_sweep_count;
            RegionAxisLinkEntry::attach_matching_points(*itr, *jtr);
        }
    }
    if constexpr (k_report_maximum_sort_and_sweep) {
        if (sort_sweep_count > s_sort_sweep_max) {
            auto entries_copy = entries;
            entries_copy.resize( std::min( std::size_t(100), entries.size() ) );
            s_sort_sweep_max = sort_sweep_count;
            std::cout << "New sort sweep maximum: " << sort_sweep_count << std::endl;
        }
    }
    return std::move(entries);
}

RegionAxisLinksAdder::RegionAxisLinksAdder
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_axis(axis_),
    m_entries(verify_entries(std::move(entries_))) {}

void RegionAxisLinksAdder::add(const SharedPtr<TriangleLink> & link_ptr) {
    if (m_axis == RegionAxis::uninitialized) {
        throw RuntimeError{":c"};
    }
    m_entries.push_back(RegionAxisLinkEntry::computed_bounds(link_ptr, m_axis));
}

RegionAxisLinksContainer RegionAxisLinksAdder::finish() {
    return RegionAxisLinksContainer
        {sort_and_sweep(dedupelicate(std::move(m_entries))), m_axis};
}

/* private static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksAdder::verify_entries
    (std::vector<RegionAxisLinkEntry> && entries)
{
#   if MACRO_DEBUG
    if (std::any_of(entries.begin(), entries.end(), RegionAxisLinkEntry::linkless)) {
        throw InvalidArgument{":c"};
    }
#   endif
    return std::move(entries);
}

// ----------------------------------------------------------------------------

/* static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksRemover::null_out_dupelicates
    (std::vector<RegionAxisLinkEntry> && entries)
{
    if (entries.size() < 2) return std::move(entries);
    std::sort(entries.begin(), entries.end(), Entry::pointer_less_than);

    auto sequence_begin = entries.begin();
    for (auto itr = entries.begin() + 1; itr != entries.end(); ++itr) {
        if (itr->link() == sequence_begin->link()) continue;

        clear_more_than_one_entry(sequence_begin, itr);
        sequence_begin = itr;
    }
    clear_more_than_one_entry(sequence_begin, entries.end());
    return std::move(entries);
}

/* static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksRemover::remove_nulls
    (std::vector<RegionAxisLinkEntry> && entries)
{
    entries.erase
        (std::remove_if(entries.begin(), entries.end(), Entry::linkless),
         entries.end());
    return std::move(entries);
}

RegionAxisLinksRemover::RegionAxisLinksRemover
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_axis(axis_),
    m_entries(verify_entries(std::move(entries_)))
{
    if constexpr (k_verify_removals_exist_in_container) {
        m_original_size = m_entries.size();
        std::sort(m_entries.begin(), m_entries.end(), Entry::pointer_less_than);
    }
}

void RegionAxisLinksRemover::add(const SharedPtr<TriangleLink> & link_ptr) {
    if constexpr (k_verify_removals_exist_in_container) {
        auto found = generalize_seek<Entry>
            (m_entries.begin(), m_entries.begin() + m_original_size,
             [&link_ptr] (const Entry & entry)
             { return static_cast<int>(link_ptr.get() - entry.link().get()); });
        if (!found) return;
        verify_in_entries(link_ptr);
    }
    m_entries.emplace_back(link_ptr);
}

RegionAxisLinksContainer RegionAxisLinksRemover::finish() {
    return RegionAxisLinksContainer
        {remove_nulls(null_out_dupelicates(std::move(m_entries))), m_axis};
}

/* private static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksRemover::verify_entries
    (std::vector<RegionAxisLinkEntry> && entries)
{
#   if MACRO_DEBUG
    std::sort(entries.begin(), entries.end(), Entry::pointer_less_than);
    auto itr = std::unique(entries.begin(), entries.end(), Entry::pointer_equal);
    if (itr != entries.end()) {
        throw InvalidArgument{":c"};
    }
#   endif
    return std::move(entries);
}

/* private static */ void RegionAxisLinksRemover::clear_more_than_one_entry
    (EntryIterator begin, EntryIterator end)
{
    if (end - begin < 2) return;
    std::fill(begin, end, Entry{nullptr});
}

/* private */ void RegionAxisLinksRemover::verify_in_entries
    (const SharedPtr<TriangleLink> & link_ptr)
{
    if constexpr (k_verify_removals_exist_in_container) {
        assert(generalize_seek<Entry>
            (m_entries.begin(), m_entries.begin() + m_original_size,
             [&link_ptr] (const Entry & entry)
             { return static_cast<int>(link_ptr.get() - entry.link().get()); }));
    }
}

namespace {

template <typename ItemType, typename IteratorType, typename ComparisonFunc>
ItemType * generalize_seek
    (IteratorType begin, IteratorType end, ComparisonFunc && compare)
{
    while (begin != end) {
        auto mid = begin + (end - begin) / 2;
        auto res = [compare = std::move(compare), &mid] {
            const ItemType & mid_item = *mid;
            return compare(mid_item);
        } ();
        if (res == 0) {
            return &*mid;
        } else if (res < 0) {
            end = mid;
        } else {
            begin = mid + 1;
        }
    }
    return nullptr;
}

} // end of <anonymous> namespace
