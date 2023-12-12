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
#include "RegionAxisAddressAndSide.hpp"

#include "../TriangleLink.hpp"
#include "../Configuration.hpp"

#include <iostream>

namespace {

using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;
using EntryContainer = RegionEdgeConnectionsContainerBase::EntryContainer;

constexpr bool k_enable_console_logging =
    k_report_tile_region_loads_and_unloads;

} // end of <anonymous> namespace

const char * axis_to_string(RegionAxis axis) {
    using Axis = RegionAxis;
    switch (axis) {
    case Axis::x_ways: return "x_ways";
    case Axis::z_ways: return "z_ways";
    default: return "uninitialized";
    }
}

// ----------------------------------------------------------------------------

/* protected static */
    const RegionEdgeConnectionsContainerBase::EntryContainer
    RegionEdgeConnectionsContainerBase::s_default_entry_container =
    RegionEdgeConnectionsContainerBase::EntryContainer{RegionAxisAddress{}};

// ----------------------------------------------------------------------------

RegionEdgeConnectionsAdder::RegionEdgeConnectionsAdder
    (EntryContainer && entries):
    m_entries(verify_all_adders("RegionEdgeConnectionsAdder", std::move(entries)))
{}

void RegionEdgeConnectionsAdder::add
    (const Vector2I & on_field_position,
     const ScaledTriangleViewGrid & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Region added at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }
    auto addresses_and_sides = triangle_grid.
        sides_and_addresses_at(on_field_position);
    for (auto & res : addresses_and_sides) {
        auto & adder = ensure_adder(res.address());
        triangle_grid.for_each_link_on_side
            (res.side(),
             [&adder](const SharedPtr<TriangleLink> & link_ptr) { adder.add(link_ptr); });
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsAdder::finish() {
    for (auto entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksAdder>(&entry.second)->
            finish();
    }
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsAdder::verify_all_adders
    (const char *, EntryContainer && entries)
{
    for (auto entry : entries)
        { (void)std::get<RegionAxisLinksAdder>(entry.second); }
    return std::move(entries);
}

/* private */ RegionAxisLinksAdder &
    RegionEdgeConnectionsAdder::ensure_adder
    (const RegionAxisAddress & addr)
{
    RegionAxisLinksAdder new_adder
        {std::vector<RegionAxisLinkEntry>{}, addr.axis()}; // LoD
    auto itr = m_entries.emplace(addr, std::move(new_adder)).position;
    return *std::get_if<RegionAxisLinksAdder>(&itr->second);
}

// ----------------------------------------------------------------------------

RegionEdgeConnectionsRemover::RegionEdgeConnectionsRemover
    (EntryContainer && entries):
    m_entries(verify_all_removers(std::move(entries)))
{}

void RegionEdgeConnectionsRemover::remove_region
    (const Vector2I & on_field_position,
     const ScaledTriangleViewGrid & triangle_grid)
{
    if constexpr (k_enable_console_logging) {
        std::cout << "Removed region at " << on_field_position.x << ", "
                  << on_field_position.y << std::endl;
    }

    auto addresses_and_sides = triangle_grid.
        sides_and_addresses_at(on_field_position);
    for (auto & res : addresses_and_sides) {
        auto * remover = find_remover(res.address());
        assert(remover);
        triangle_grid.for_each_link_on_side
            (res.side(),
             [remover]
             (const SharedPtr<TriangleLink> & linkptr)
             { remover->add(linkptr); });
    }
}

RegionEdgeConnectionsContainer RegionEdgeConnectionsRemover::finish() {
    for (auto entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksRemover>(&entry.second)->
            finish(entry.first);
    }
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsRemover::verify_all_removers
    (EntryContainer && entries)
{
    for (auto entry : entries)
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
    for (auto entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksContainer>(&entry.second)->
            make_adder();
    }
    return RegionEdgeConnectionsAdder{std::move(m_entries)};
}

RegionEdgeConnectionsRemover
    RegionEdgeConnectionsContainer::make_remover()
{
    for (auto entry : m_entries) {
        entry.second = std::get_if<RegionAxisLinksContainer>(&entry.second)->
            make_remover();
    }
    return RegionEdgeConnectionsRemover{std::move(m_entries)};
}

/* private static */ EntryContainer
    RegionEdgeConnectionsContainer::verify_containers
    (EntryContainer && entries)
{
    for (auto entry : entries)
        { (void)std::get<RegionAxisLinksContainer>(entry.second); }
    return std::move(entries);
}
