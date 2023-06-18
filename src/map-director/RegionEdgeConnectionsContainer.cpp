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

namespace {

using namespace cul::exceptions_abbr;
using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

constexpr bool k_enable_console_logging = true;

Real get_x(const Vector & r) { return r.x; }

Real get_z(const Vector & r) { return r.z; }

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


void RegionAxisLinkEntry::attempt_attachment_to(const RegionAxisLinkEntry & rhs)
    { (void)m_link_ptr->attempt_attachment_to(rhs.link()); }

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
    for_each_tile_on_edge
        (RectangleI{Vector2I{}, triangle_grid.size2()}, side,
         [&triangle_grid, &adder] (int x, int y)
        {
            for (auto & link : triangle_grid(x, y))
                adder.add(link);
        });
}

RegionAxisLinksAdder::RegionAxisLinksAdder
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_axis(axis_),
    m_entries(verify_entries(std::move(entries_)))
{}

void RegionAxisLinksAdder::add(const SharedPtr<TriangleLink> & link_ptr) {
    // verify axis is initialized
    m_entries.push_back(RegionAxisLinkEntry::computed_bounds(link_ptr, m_axis));
}

RegionAxisLinksContainer RegionAxisLinksAdder::finish() {
    std::sort(m_entries.begin(), m_entries.end(),
              RegionAxisLinkEntry::pointer_less_than);
    auto uend = std::unique
        (m_entries.begin(), m_entries.end(), RegionAxisLinkEntry::pointer_equal);
    m_entries.erase(uend, m_entries.end());
    // sort and sweep
    std::sort(m_entries.begin(), m_entries.end(),
              RegionAxisLinkEntry::bounds_less_than);
    for (auto itr = m_entries.begin(); itr != m_entries.end(); ++itr) {
        for (auto jtr = itr + 1; jtr != m_entries.end(); ++jtr) {
            if (itr->high_bounds() < jtr->low_bounds())
                break;
            itr->attempt_attachment_to(*jtr);
            jtr->attempt_attachment_to(*itr);
        }
    }
    return RegionAxisLinksContainer{std::move(m_entries), m_axis};
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

RegionAxisLinksRemover::RegionAxisLinksRemover
    (std::vector<RegionAxisLinkEntry> && entries_,
     RegionAxis axis_):
    m_axis(axis_),
    m_entries(verify_entries(std::move(entries_)))
{}

void RegionAxisLinksRemover::add(const SharedPtr<TriangleLink> & link_ptr) {
    m_entries.emplace_back(link_ptr);
}

RegionAxisLinksContainer RegionAxisLinksRemover::finish() {
    // null_out_dupelicates
    std::sort(m_entries.begin(), m_entries.end(),
              RegionAxisLinkEntry::pointer_less_than);
    SharedPtr<TriangleLink> last;
    for (auto & entry : m_entries) {
        bool is_to_clear_link = entry.link() == last;
        last = entry.link();
        if (is_to_clear_link) {
            entry.set_link_to_null();
        }
    }

    // remove_nulls
    m_entries.erase
        (std::remove_if(m_entries.begin(), m_entries.end(), Entry::linkless),
         m_entries.end());

    return RegionAxisLinksContainer{std::move(m_entries), m_axis};
}

/* private static */ std::vector<RegionAxisLinkEntry>
    RegionAxisLinksRemover::verify_entries
    (std::vector<RegionAxisLinkEntry> && entries)
{
#   if MACRO_DEBUG
    using Entry = RegionAxisLinkEntry;
    std::sort(entries.begin(), entries.end(), Entry::pointer_less_than);
    auto itr = std::unique(entries.begin(), entries.end(), Entry::pointer_equal);
    if (itr != entries.end()) {
        throw InvArg{":c"};
    }
#   endif
    return std::move(entries);
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

int RegionAxisAddress::compare(const RegionAxisAddress & rhs) const {
    if (m_axis == rhs.m_axis)
        { return m_value - rhs.m_value; }
    return static_cast<int>(m_axis) - static_cast<int>(rhs.m_axis);
}

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

// ----------------------------------------------------------------------------

/* protected static */ RegionEdgeConnectionsContainerBase::ChangeEntryContainer
    RegionEdgeConnectionsContainerBase::verify_change_entries
    (ChangeEntryContainer && entries)
{
    if (entries.empty()) return std::move(entries);
    throw InvArg{":c"};
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsAdder::RegionEdgeConnectionsAdder
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

void RegionEdgeConnectionsAdder::add
    (const Vector2I & on_field_position,
     const SharedPtr<ViewGridTriangle> & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Removed added at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }
    auto addresses_and_sides = RegionAxisAddressAndSide::for_
        (on_field_position, triangle_grid->size2());
    for (auto & res : addresses_and_sides) {
        m_change_entries.emplace_back(res.side(), res.address(), triangle_grid);
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsAdder::finish() {
    // some method named "ensure_entry_for_changes"
    auto old_size = m_entries.size();
    for (const auto & change : m_change_entries) {
        auto * entry = Entry::seek
            (change.address(), m_entries.begin(), m_entries.begin() + old_size);
        if (!entry) {
            // have to make one
            Entry new_entry;
            new_entry.address = change.address();
            new_entry.axis_container =
                std::get<RegionAxisLinksContainer>(new_entry.axis_container).
                make_adder(side_to_axis(change.grid_side()));
            m_entries.push_back(new_entry);
        }
    }

    // apply_changes
    std::sort(m_entries.begin(), m_entries.end(), Entry::less_than);
    for (const auto & change : m_change_entries) {
        auto * entry = Entry::seek(change.address(), m_entries.begin(), m_entries.end());
        assert(entry);
        auto * adder = std::get_if<RegionAxisLinksAdder>(&entry->axis_container);
        RegionAxisLinksAdder::add_sides_from
            (change.grid_side(), *change.triangle_grid(), *adder);
    }
    m_change_entries.clear();

    // finish_adders
    for (auto & entry : m_entries) {
        entry.axis_container =
            std::get_if<RegionAxisLinksAdder>(&entry.axis_container)->
            finish();
    }

    return RegionEdgeConnectionsContainer
        {std::move(m_change_entries), std::move(m_entries)};
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsRemover::RegionEdgeConnectionsRemover
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

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
        m_change_entries.emplace_back(res.side(), res.address(), triangle_grid);
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsRemover::finish() {
    for (auto & change : m_change_entries) {
        auto * entry =
            Entry::seek(change.address(), m_entries.begin(), m_entries.end());
        auto * remover = std::get_if<RegionAxisLinksRemover>(&entry->axis_container);
        for_each_tile_on_edge
            (RectangleI{Vector2I{}, change.triangle_grid()->size2()},
             change.grid_side(),
             [remover, &change](int x, int y)
        {
            for (auto & linkptr : (*change.triangle_grid())(x, y))
                remover->add(linkptr);
        });
    }
    m_change_entries.clear();

    for (auto & entry : m_entries) {
        entry.axis_container =
            std::get_if<RegionAxisLinksRemover>(&entry.axis_container)->
            finish();
    }
    return RegionEdgeConnectionsContainer
        {std::move(m_change_entries), std::move(m_entries)};
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsContainer::RegionEdgeConnectionsContainer
    (ChangeEntryContainer && change_entries_,
     EntryContainer && entries):
    // assumes changes are empty
    // assumes all entries contain usual axis container types
    m_change_entries(verify_change_entries(std::move(change_entries_))),
    m_entries(std::move(entries)) {}

RegionEdgeConnectionsAdder RegionEdgeConnectionsContainer::make_adder() {
    for (auto & entry : m_entries) {
        entry.axis_container =
            std::get_if<RegionAxisLinksContainer>(&entry.axis_container)->
            make_adder();
    }
    return RegionEdgeConnectionsAdder
        {std::move(m_change_entries), std::move(m_entries)};
}

RegionEdgeConnectionsRemover RegionEdgeConnectionsContainer::make_remover() {
    for (auto & entry : m_entries) {
        entry.axis_container =
            std::get_if<RegionAxisLinksContainer>(&entry.axis_container)->
            make_remover();
    }
    return RegionEdgeConnectionsRemover
        {std::move(m_change_entries), std::move(m_entries)};
}
