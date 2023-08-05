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

#include "RegionEdgeConnectionsContainer.hpp"

#include "../TriangleLink.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;
using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;
using EntryContainer = RegionEdgeConnectionsContainerBase::EntryContainer;

constexpr bool k_enable_console_logging = true;

Real get_x(const Vector & r) { return r.x; }

Real get_z(const Vector & r) { return r.z; }

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

RegionAxis side_to_axis(RegionSide side) {
    using Side = RegionSide;
    using Axis = RegionAxis;
    switch (side) {
    case Side::bottom: case Side::top  : return Axis::x_ways;
    case Side::left  : case Side::right: return Axis::z_ways;
    default: return Axis::uninitialized;
    }
}

const char * axis_to_string(RegionAxis axis) {
    using Axis = RegionAxis;
    switch (axis) {
    case Axis::x_ways: return "x_ways";
    case Axis::z_ways: return "z_ways";
    default: return "uninitialized";
    }
}

// ----------------------------------------------------------------------------

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
{ TriangleLink::attach_matching_points(lhs.link(), rhs.link()); }

/* static */ RegionAxisLinkEntry RegionAxisLinkEntry::computed_bounds
    (const SharedPtr<TriangleLink> & link_ptr, RegionAxis axis)
{
    using Axis = RegionAxis;
    switch (axis) {
    case Axis::x_ways: return computed_bounds_<get_x>(link_ptr);
    case Axis::z_ways: return computed_bounds_<get_z>(link_ptr);
    default          : break;
    }
    throw RtError{":c"};
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

/* static */ void RegionAxisLinksAdder::add_sides_from
    (RegionSide side,
     const ViewGridTriangle & triangle_grid,
     RegionAxisLinksAdder & adder)
{
    auto add_sides_for_tile = [&triangle_grid, &adder] (int x, int y) {
        for (auto & link : triangle_grid(x, y))
            adder.add(link);
    };

    for_each_tile_on_edge(triangle_grid, side, add_sides_for_tile);
}

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
#   if 1
    static std::size_t max_count = 0;
    if (max_count < entries.size()) {
        max_count = entries.size();
        std::cout << "New sort and sweep max: " << max_count << std::endl;
    }
#   endif
    std::sort(entries.begin(), entries.end(),
              RegionAxisLinkEntry::bounds_less_than);
    for (auto itr = entries.begin(); itr != entries.end(); ++itr) {
        for (auto jtr = itr + 1; jtr != entries.end(); ++jtr) {
            if (itr->high_bounds() < jtr->low_bounds())
                break;
            RegionAxisLinkEntry::attach_matching_points(*itr, *jtr);
        }
    }
    return std::move(entries);
}

RegionAxisLinksAdder::RegionAxisLinksAdder
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_axis(axis_),
    m_entries(verify_entries(std::move(entries_)))
{}

void RegionAxisLinksAdder::add(const SharedPtr<TriangleLink> & link_ptr) {
    if (m_axis == RegionAxis::uninitialized) {
        throw RtError{":c"};
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
        throw InvArg{":c"};
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

    auto last = entries[1].link();
    if (entries[0].link() == entries[1].link())
        { entries[0] = Entry{nullptr}; }

    for (auto itr = entries.begin() + 1; itr != entries.end(); ++itr) {
        auto next_last = itr->link();
        if (itr->link() == last)
            { *itr = Entry{nullptr}; }
        last = std::move(next_last);
    }

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
        throw InvArg{":c"};
    }
#   endif
    return std::move(entries);
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

// ----------------------------------------------------------------------------

RegionAxisLinksContainer::RegionAxisLinksContainer
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_entries(std::move(entries_)),
    m_axis(axis_) {}

RegionAxisLinksRemover RegionAxisLinksContainer::make_remover() {
    return RegionAxisLinksRemover{std::move(m_entries), m_axis};
}

RegionAxisLinksAdder RegionAxisLinksContainer::make_adder(RegionAxis axis_) {
    return RegionAxisLinksAdder{std::move(m_entries), axis_};
}

RegionAxisLinksAdder RegionAxisLinksContainer::make_adder() {
    return RegionAxisLinksAdder{std::move(m_entries), m_axis};
}

// ----------------------------------------------------------------------------

bool RegionAxisAddress::operator < (const RegionAxisAddress & rhs) const
    { return compare(rhs) < 0; }

bool RegionAxisAddress::operator == (const RegionAxisAddress & rhs) const
    { return compare(rhs) == 0; }

int RegionAxisAddress::compare(const RegionAxisAddress & rhs) const {
    if (m_axis == rhs.m_axis)
        { return m_value - rhs.m_value; }
    return static_cast<int>(m_axis) - static_cast<int>(rhs.m_axis);
}

/* new */ std::size_t RegionAxisAddress::hash() const
    { return std::hash<int>{}(m_value) ^ std::hash<RegionAxis>{}(m_axis); }

// ----------------------------------------------------------------------------

/* static */ std::array<RegionAxisAddressAndSide, 4>
    RegionAxisAddressAndSide::for_
    (const Vector2I & on_field, const Size2I & grid_size)
{
    using Side = RegionSide;
    using Axis = RegionAxis;
    auto bottom = on_field.y + grid_size.height;
    auto right  = on_field.x + grid_size.width ;
    return {
        RegionAxisAddressAndSide{Axis::x_ways, on_field.x, Side::left  },
        RegionAxisAddressAndSide{Axis::x_ways, right     , Side::right },
        RegionAxisAddressAndSide{Axis::z_ways, on_field.y, Side::top   },
        RegionAxisAddressAndSide{Axis::z_ways, bottom    , Side::bottom}
    };
}

bool RegionAxisAddressAndSide::operator ==
    (const RegionAxisAddressAndSide & rhs) const
{
    return address().compare(rhs.address()) == 0 &&
           side() == rhs.side();
}

// ----------------------------------------------------------------------------
#if 0
/* static */ bool RegionEdgeConnectionEntry::less_than
    (const RegionEdgeConnectionEntry & lhs,
     const RegionEdgeConnectionEntry & rhs)
{ return lhs.address() < rhs.address(); }

/* static */ RegionEdgeConnectionEntry * RegionEdgeConnectionEntry::seek
    (const RegionAxisAddress & saught, Iterator begin, Iterator end)
{
    return generalize_seek<RegionEdgeConnectionEntry>
        (begin, end,
         [&saught] (const RegionEdgeConnectionEntry & entry)
        { return saught.compare(entry.address()); });
}

/* static */ RegionEdgeConnectionEntry RegionEdgeConnectionEntry::start_as_adder
    (RegionAxisAddress address_, RegionAxis axis_)
{
    return RegionEdgeConnectionEntry
        {cul::TypeTag<RegionAxisLinksAdder>{}, address_, axis_};
}

RegionAxisLinksAdder * RegionEdgeConnectionEntry::as_adder()
    { return std::get_if<RegionAxisLinksAdder>(&m_axis_container); }

RegionAxisLinksRemover * RegionEdgeConnectionEntry::as_remover()
    { return std::get_if<RegionAxisLinksRemover>(&m_axis_container); }

void RegionEdgeConnectionEntry::set_container(RegionAxisLinksContainer && t)
    { m_axis_container = std::move(t); }

void RegionEdgeConnectionEntry::reset_as_adder() {
    m_axis_container =
        std::get_if<RegionAxisLinksContainer>(&m_axis_container)->
        make_adder();
}

void RegionEdgeConnectionEntry::reset_as_remover() {
    m_axis_container =
        std::get_if<RegionAxisLinksContainer>(&m_axis_container)->
        make_remover();
}

/* private */ RegionEdgeConnectionEntry::RegionEdgeConnectionEntry
    (cul::TypeTag<RegionAxisLinksAdder>,
     RegionAxisAddress address_,
     RegionAxis axis_):
    m_address(address_),
    m_axis_container(std::in_place_type_t<RegionAxisLinksAdder>{},
                     std::vector<RegionAxisLinkEntry>{}, axis_)
{}
#endif
// ----------------------------------------------------------------------------
#if 0
/* protected static */ RegionEdgeConnectionsContainerBase::ChangeEntryContainer
    RegionEdgeConnectionsContainerBase::verify_change_entries
    (ChangeEntryContainer && entries)
{
    if (entries.empty()) return std::move(entries);
    throw InvArg{":c"};
}
#endif
/* protected static */
    const RegionEdgeConnectionsContainerBase::EntryContainer
    RegionEdgeConnectionsContainerBase::s_default_entry_container =
    RegionEdgeConnectionsContainerBase::EntryContainer{2, RegionAxisAddress{}};

// ----------------------------------------------------------------------------
#if 0
/* static */ RegionEdgeConnectionsAdder_Old::EntryContainer
    RegionEdgeConnectionsAdder_Old::ensure_entries_for_changes
    (const ChangeEntryContainer & changes, EntryContainer && entries)
{
    auto old_size = entries.size();
    for (const auto & change : changes) {
        auto * entry = Entry::seek
            (change.address(), entries.begin(), entries.begin() + old_size);
        if (entry) { continue; }

        // have to make one
        entries.emplace_back
            (Entry::start_as_adder(change.address(),
                                   side_to_axis(change.grid_side())));
    }
    return std::move(entries);
}

/* static */ RegionEdgeConnectionsAdder_Old::EntryContainer
    RegionEdgeConnectionsAdder_Old::apply_additions
    (const ChangeEntryContainer & changes, EntryContainer && entries)
{
    std::sort(entries.begin(), entries.end(), Entry::less_than);
    for (const auto & change : changes) {
        auto * entry = Entry::seek
            (change.address(), entries.begin(), entries.end());
        assert(entry);
#       if 0
        std::cout << axis_to_string(change.address().axis()) << " " << change.address().value() << std::endl;
#       endif
        RegionAxisLinksAdder::add_sides_from
            (change.grid_side(), *change.triangle_grid(), *entry->as_adder());
    }
    return std::move(entries);
}

/* static */ RegionEdgeConnectionsAdder_Old::EntryContainer
    RegionEdgeConnectionsAdder_Old::finish_adders(EntryContainer && entries)
{
    for (auto & entry : entries) {
        entry.set_container(entry.as_adder()->finish());
    }
    return std::move(entries);
}

RegionEdgeConnectionsAdder_Old::RegionEdgeConnectionsAdder_Old
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

void RegionEdgeConnectionsAdder_Old::add
    (const Vector2I & on_field_position,
     const SharedPtr<ViewGridTriangle> & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Region added at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }

    auto addresses_and_sides = RegionAxisAddressAndSide::for_
        (on_field_position, triangle_grid->size2());
    for (auto & res : addresses_and_sides) {
        m_change_entries.emplace_back(res.side(), res.address(), triangle_grid);
    }
}

RegionEdgeConnectionsContainer_Old RegionEdgeConnectionsAdder_Old::finish() {
    m_entries = ensure_entries_for_changes(m_change_entries, std::move(m_entries));
    m_entries = apply_additions           (m_change_entries, std::move(m_entries));
    m_entries = finish_adders             (                  std::move(m_entries));
    m_change_entries.clear();

    // finishing becomes trivial

    return RegionEdgeConnectionsContainer_Old
        {std::move(m_change_entries), std::move(m_entries)};
}
#endif
// ----------------------------------------------------------------------------
#if 0
/* static */ RegionEdgeConnectionsRemover_Old::EntryContainer
    RegionEdgeConnectionsRemover_Old::apply_removals
    (const ChangeEntryContainer & changes, EntryContainer && entries)
{
    for (auto & change : changes) {
        auto * entry =
            Entry::seek(change.address(), entries.begin(), entries.end());
        for_each_tile_on_edge
            (*change.triangle_grid(), change.grid_side(),
             [entry, &change](int x, int y)
        {
            for (auto & linkptr : (*change.triangle_grid())(x, y))
                entry->as_remover()->add(linkptr);
        });
    }
    return std::move(entries);
}

/* static */ RegionEdgeConnectionsRemover_Old::EntryContainer
    RegionEdgeConnectionsRemover_Old::finish_removers(EntryContainer && entries)
{
    for (auto & entry : entries) {
        entry.set_container(entry.as_remover()->finish());
    }
    return std::move(entries);
}

RegionEdgeConnectionsRemover_Old::RegionEdgeConnectionsRemover_Old
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

void RegionEdgeConnectionsRemover_Old::remove_region
    (const Vector2I & on_field_position,
     const SharedPtr<ViewGridTriangle> & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Removed region at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }

    auto addresses_and_sides = RegionAxisAddressAndSide::for_
        (on_field_position, triangle_grid->size2());
    for (auto & res : addresses_and_sides) {
        m_change_entries.emplace_back(res.side(), res.address(), triangle_grid);
    }
}

RegionEdgeConnectionsContainer_Old RegionEdgeConnectionsRemover_Old::finish() {
    m_entries = apply_removals (m_change_entries, std::move(m_entries));
    m_entries = finish_removers(std::move(m_entries));
    m_change_entries.clear();

    return RegionEdgeConnectionsContainer_Old
        {std::move(m_change_entries), std::move(m_entries)};
}
#endif
// ----------------------------------------------------------------------------
#if 0
RegionEdgeConnectionsContainer_Old::RegionEdgeConnectionsContainer_Old
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    // assumes changes are empty
    // assumes all entries contain usual axis container types
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

RegionEdgeConnectionsAdder_Old RegionEdgeConnectionsContainer_Old::make_adder() {
    for (auto & entry : m_entries) {
        entry.reset_as_adder();
    }
    return RegionEdgeConnectionsAdder_Old
        {std::move(m_change_entries), std::move(m_entries)};
}

RegionEdgeConnectionsRemover_Old RegionEdgeConnectionsContainer_Old::make_remover() {
    for (auto & entry : m_entries) {
        entry.reset_as_remover();
    }
    return RegionEdgeConnectionsRemover_Old
        {std::move(m_change_entries), std::move(m_entries)};
}
#endif
// New Stuff
// ----------------------------------------------------------------------------

RegionEdgeConnectionsAdder::RegionEdgeConnectionsAdder
    (EntryContainer && entries):
    m_entries(verify_all_adders("RegionEdgeConnectionsAdder", std::move(entries)))
{}

void RegionEdgeConnectionsAdder::add
    (const Vector2I & on_field_position,
     const SharedPtr<ViewGridTriangle> & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Region added at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }

    auto addresses_and_sides = RegionAxisAddressAndSide::for_
        (on_field_position, triangle_grid->size2());
    for (auto & res : addresses_and_sides) {
        auto & adder = ensure_adder(res.address());
        for_each_tile_on_edge
            (*triangle_grid, res.side(),
             [&adder, &triangle_grid] (int x, int y)
        {
            for (auto & triangle_link : (*triangle_grid)(x, y))
                { adder.add(triangle_link); }
        });
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsAdder::finish() {
    for (auto & entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksAdder>(&entry.second)->
            finish();
    }
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsAdder::verify_all_adders
    (const char *, EntryContainer && entries)
{
    for (auto & entry : entries)
        { (void)std::get<RegionAxisLinksAdder>(entry.second); }
    return std::move(entries);
}

/* private */ RegionAxisLinksAdder &
    RegionEdgeConnectionsAdder::ensure_adder
    (const RegionAxisAddress & addr)
{
    RegionAxisLinksAdder new_adder
        {std::vector<RegionAxisLinkEntry>{}, addr.axis()}; // LoD
    auto itr = m_entries.ensure(addr, std::move(new_adder));
    return *std::get_if<RegionAxisLinksAdder>(&itr->second);
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsRemover::RegionEdgeConnectionsRemover
    (EntryContainer && entries):
    m_entries(verify_all_removers(std::move(entries)))
{}

void RegionEdgeConnectionsRemover::remove_region
    (const Vector2I & on_field_position,
     const SharedPtr<ViewGridTriangle> & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Removed region at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }

    auto addresses_and_sides = RegionAxisAddressAndSide::for_
        (on_field_position, triangle_grid->size2());
    for (auto & res : addresses_and_sides) {
        auto * remover = find_remover(res.address());
        assert(remover);
        for_each_tile_on_edge(*triangle_grid, res.side(), [&](int x, int y)
        {
            for (auto & linkptr : (*triangle_grid)(x, y))
                remover->add(linkptr);
        });
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsRemover::finish() {
    for (auto & entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksRemover>(&entry.second)->
            finish();
    }
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsRemover::verify_all_removers
    (EntryContainer && entries)
{
    for (auto & entry : entries)
        { (void)std::get<RegionAxisLinksRemover>(entry.second); }
    return std::move(entries);
}

/* private */ RegionAxisLinksRemover *
    RegionEdgeConnectionsRemover::find_remover
    (const RegionAxisAddress & addr)
{
    auto entry = m_entries.find(addr);
    if (entry == m_entries.end()) return nullptr;
    return std::get_if<RegionAxisLinksRemover>(&entry->second);
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsContainer::RegionEdgeConnectionsContainer
    (EntryContainer && entries):
    m_entries(verify_containers(std::move(entries))) {}

RegionEdgeConnectionsAdder
    RegionEdgeConnectionsContainer::make_adder()
{
    for (auto & entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksContainer>(&entry.second)->
            make_adder();
    }
    return RegionEdgeConnectionsAdder{std::move(m_entries)};
}

RegionEdgeConnectionsRemover
    RegionEdgeConnectionsContainer::make_remover()
{
    for (auto & entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksContainer>(&entry.second)->
            make_remover();
    }
    return RegionEdgeConnectionsRemover{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsContainer::verify_containers
    (EntryContainer && entries)
{
    for (auto & entry : entries)
        { (void)std::get<RegionAxisLinksContainer>(entry.second); }
    return std::move(entries);
}
