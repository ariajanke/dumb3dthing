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

using ViewGridTriangle = RegionEdgeLinksContainer::ViewGridTriangle;

} // end of <anonymous> namespace

RegionEdgeLinksContainer::RegionEdgeLinksContainer
    (const ViewGridTriangle & views)
{
    append_links_by_predicate<is_not_edge_tile>(views, m_links);
    auto idx_for_edge = m_links.size();
    append_links_by_predicate<is_edge_tile>(views, m_links);
    m_edge_begin = m_links.begin() + idx_for_edge;
}

void RegionEdgeLinksContainer::glue_to
    (RegionEdgeLinksContainer & rhs)
{
    for (auto itr = edge_begin(); itr != edge_end(); ++itr) {
        for (auto jtr = rhs.edge_begin(); jtr != rhs.edge_end(); ++jtr) {
            (**itr).attempt_attachment_to(*jtr);
            (**jtr).attempt_attachment_to(*itr);
        }
    }
}

/* private static */ bool RegionEdgeLinksContainer::is_edge_tile
    (const ViewGridTriangle & grid, const Vector2I & r)
{
    return std::any_of
        (k_plus_shape_neighbor_offsets.begin(),
         k_plus_shape_neighbor_offsets.end(),
         [&] (const Vector2I & offset)
         { return !grid.has_position(offset + r); });
}

/* private static */ bool RegionEdgeLinksContainer::is_not_edge_tile
    (const ViewGridTriangle & grid, const Vector2I & r)
    { return !is_edge_tile(grid, r); }

template <bool (*meets_pred)(const ViewGridTriangle &, const Vector2I &)>
/* private static */ void RegionEdgeLinksContainer::append_links_by_predicate
    (const ViewGridTriangle & views, std::vector<SharedPtr<TriangleLink>> & links)
{
    for (Vector2I r; r != views.end_position(); r = views.next(r)) {
        if (!meets_pred(views, r)) continue;
        for (auto & link : views(r)) {
            links.push_back(link);
        }
    }
}

// ----------------------------------------------------------------------------

/* static */ std::array<RegionSideAddress, 4> RegionSideAddress::addresses_for
    (const Vector2I & on_field_position, const Size2I & grid_size)
{
    auto right  = on_field_position.x + grid_size.width ;
    auto bottom = on_field_position.y + grid_size.height;
    return {
        RegionSideAddress{ RegionSide::top   , on_field_position.y },
        RegionSideAddress{ RegionSide::bottom, bottom              },
        RegionSideAddress{ RegionSide::left  , on_field_position.x },
        RegionSideAddress{ RegionSide::right , right               },
    };
}

int RegionSideAddress::compare(const RegionSideAddress & rhs) const {
    if (side() != rhs.side())
        { return static_cast<int>(side()) - static_cast<int>(rhs.side()); }
    return value() - rhs.value();
}

/* private */ RegionSide RegionSideAddress::other_side() const {
    switch (m_side) {
    case RegionSide::bottom: return RegionSide::top   ;
    case RegionSide::left  : return RegionSide::right ;
    case RegionSide::right : return RegionSide::left  ;
    case RegionSide::top   : return RegionSide::bottom;
    }
}

std::size_t RegionSideAddressHasher::operator ()
    (const RegionSideAddress & address) const
{
    using IntHasher = std::hash<int>;
    using SideHasher = std::hash<RegionSide>;
    return IntHasher{}(address.value()) ^ SideHasher{}(address.side());
}

// ----------------------------------------------------------------------------

/* static */ bool RegionEdgeConnectionEntry::less_than
    (const RegionEdgeConnectionEntry & lhs,
     const RegionEdgeConnectionEntry & rhs)
{ return lhs.address().compare(rhs.address()) < 0; }

/* static */ RegionEdgeConnectionEntry::Container
    RegionEdgeConnectionEntry::verify_sorted
    (const char * constructor_name, Container && container)
{
    using namespace cul::exceptions_abbr;
    if (std::is_sorted(container.begin(), container.end(), less_than))
        { return std::move(container); }
    throw InvArg{std::string{constructor_name} + "::" +
                 std::string{constructor_name} + ": entries must be sorted"};
}

/* static */ RegionEdgeConnectionEntry * RegionEdgeConnectionEntry::seek
    (const RegionSideAddress & addr, Iterator begin, Iterator end)
{
    while (begin != end) {
        auto mid = begin + ((end - begin) / 2);
        auto res = mid->address().compare(addr);
        if (res == 0)
            { return &*mid; }
        if (res < 0) {
            begin = mid + 1;
        } else {
            end = mid;
        }
    }
    return nullptr;
}

RegionEdgeConnectionEntry::RegionEdgeConnectionEntry
    (RegionSideAddress address_,
     const SharedPtr<RegionEdgeLinksContainer> & link_container):
    m_address(address_),
    m_link_container(link_container) {}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsAdder::RegionEdgeConnectionsAdder
    (RegionEdgeConnectionEntry::Container && container):
    m_entries(RegionEdgeConnectionEntry::verify_sorted
        ("RegionEdgeConnectionsAdder", std::move(container))
    ),
    m_old_size(m_entries.size()) {}

void RegionEdgeConnectionsAdder::add
    (const Vector2I & on_field_position, const ViewGridTriangle & triangle_grid)
{
    auto addrs = RegionSideAddress::addresses_for
        (on_field_position, triangle_grid.size2());
    auto link_container_ptr =
        make_shared<RegionEdgeLinksContainer>(triangle_grid);

    for (auto & addr : addrs) {
        m_entries.emplace_back(addr, link_container_ptr);
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsAdder::finish() {
    using Entry = RegionEdgeConnectionEntry;

    auto dirty_begin = m_entries.begin() + m_old_size;
    std::sort(dirty_begin, m_entries.end(), Entry::less_than);
    std::inplace_merge
        (m_entries.begin(), dirty_begin, m_entries.end(), Entry::less_than);
    // TODO: optimize s.t. already linkeds are not checked again
    // this should be O(n log n)
    // attempt_to_attach_to should ensure correct behavior at least
    for (auto & entry : m_entries) {
        auto other_entry = Entry::seek
            (entry.address().to_other_side(), m_entries.begin(), m_entries.end());
        if (!other_entry)
            { continue; }
        other_entry->link_container()->glue_to(*entry.link_container());
    }

    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsRemover::RegionEdgeConnectionsRemover
    (RegionEdgeConnectionEntry::Container && container):
    m_entries(RegionEdgeConnectionEntry::verify_sorted
        ("RegionEdgeConnectionsRemover", std::move(container))
    ) {}

void RegionEdgeConnectionsRemover::remove_region
    (const Vector2I & on_field_position, const Size2I & grid_size)
{
    auto addrs = RegionSideAddress::addresses_for
        (on_field_position, grid_size);
    for (auto & addr : addrs) {
        if (auto entry = seek(addr)) {
            *entry = RegionEdgeConnectionEntry{addr, nullptr};
        }
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsRemover::finish() {
    std::remove_if(m_entries.begin(), m_entries.end(), has_no_link_container);
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

/* private */ RegionEdgeConnectionEntry *
    RegionEdgeConnectionsRemover::seek(const RegionSideAddress & addr)
{
    using Entry = RegionEdgeConnectionEntry;
    return Entry::seek(addr, m_entries.begin(), m_entries.end());
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsContainer::RegionEdgeConnectionsContainer
    (RegionEdgeConnectionEntry::Container && container):
    m_entries(RegionEdgeConnectionEntry::verify_sorted
        ("RegionEdgeConnectionsContainer", std::move(container))
    ) {}

RegionEdgeConnectionsAdder RegionEdgeConnectionsContainer::make_adder()
    { return RegionEdgeConnectionsAdder{std::move(m_entries)}; }

RegionEdgeConnectionsRemover RegionEdgeConnectionsContainer::make_remover()
    { return RegionEdgeConnectionsRemover{std::move(m_entries)}; }
