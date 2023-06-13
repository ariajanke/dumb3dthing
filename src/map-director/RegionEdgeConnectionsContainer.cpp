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

void RegionEdgeLink::glue_to(RegionEdgeLink & other_edge) {
    for (auto itr = other_edge.m_begin; itr != other_edge.m_end; ++itr) {
    for (auto jtr = m_begin; jtr != m_end; ++jtr) {
        (**itr).attempt_attachment_to(*jtr);
        (**jtr).attempt_attachment_to(*itr);
    }}
}

// ----------------------------------------------------------------------------

class EdgeSetterBase {
public:
    virtual ~EdgeSetterBase() {}

    virtual int strip_end(const ViewGridTriangle & views) const = 0;

    virtual Vector2I strip_position_to_v2(int strip_pos) const = 0;

    static std::size_t add_links
        (RegionSide side,
         const ViewGridTriangle & views,
         std::vector<SharedPtr<TriangleLink>> & links)
    {
        ;
    }

    std::size_t add_links(const ViewGridTriangle & views,
                          std::vector<SharedPtr<TriangleLink>> & links)
    {
        auto end_pos = strip_end(views);
        for (int strip_pos = 0; strip_pos != end_pos; ++strip_pos) {
            for (auto & link_ptr : views(strip_position_to_v2(strip_pos))) {
                links.push_back(link_ptr);
            }
        }
        return links.size();
    }
};

class VerticalEdgeSetter final : public EdgeSetterBase {
public:
    VerticalEdgeSetter(int x_pos):
        m_x_pos(x_pos) {}

    int strip_end(const ViewGridTriangle & views) const final
        { return views.height(); }

    Vector2I strip_position_to_v2(int strip_pos) const final
        { return Vector2I{m_x_pos, strip_pos}; }

private:
    int m_x_pos;
};

class HorizontalEdgeSetter final : public EdgeSetterBase {
public:
    HorizontalEdgeSetter(int y_pos):
        m_y_pos(y_pos) {}

    int strip_end(const ViewGridTriangle & views) const final
        { return views.width(); }

    Vector2I strip_position_to_v2(int strip_pos) const final
        { return Vector2I{strip_pos, m_y_pos}; }

private:
    int m_y_pos;
};

RegionEdgeLinksContainer::RegionEdgeLinksContainer
    (const ViewGridTriangle & views)
{
    std::array<std::size_t, 4> size_up_to;

    // when this gets refactored, the rest will be much easier
    auto hstrip = [this, &views] (int y_pos) {
        for (int x = 0; x != views.width(); ++x) {
            for (auto & link_ptr : views(Vector2I{x, y_pos})) {
                m_links.push_back(link_ptr);
            }
        }
        return m_links.size();
    };

    auto vstrip = [this, &views] (int x_pos) {
        for (int y = 0; y != views.height(); ++y) {
            for (auto & link_ptr : views(Vector2I{x_pos, y})) {
                m_links.push_back(link_ptr);
            }
        }
        return m_links.size();
    };



    size_up_to[static_cast<int>(RegionSide::top   )] = hstrip(0);
    size_up_to[static_cast<int>(RegionSide::bottom)] = hstrip(views.height() - 1);
    size_up_to[static_cast<int>(RegionSide::left  )] = vstrip(0);
    size_up_to[static_cast<int>(RegionSide::right )] = vstrip(views.width() - 1);

    auto begin = m_links.begin();
    std::size_t beg_idx = 0;
    auto set_edge_side = [this, begin, size_up_to] (RegionSide side, std::size_t beg_idx) {
        auto side_idx = static_cast<int>(side);
        auto next = size_up_to[side_idx];
        m_edges[side_idx] = RegionEdgeLink{begin + beg_idx, begin + next};
        return beg_idx = next;
    };
    beg_idx = set_edge_side(RegionSide::top   , beg_idx);
    beg_idx = set_edge_side(RegionSide::bottom, beg_idx);
    beg_idx = set_edge_side(RegionSide::left  , beg_idx);
    beg_idx = set_edge_side(RegionSide::right , beg_idx);

    // smell as fuck, but unclear how to unfuck
}

void RegionEdgeLinksContainer::glue_to(RegionEdgeLinksContainer & rhs) {
    glue_to(RegionSide::bottom, rhs);
    glue_to(RegionSide::top   , rhs);
    glue_to(RegionSide::right , rhs);
    glue_to(RegionSide::left  , rhs);
#   if 0
    for (auto itr = edge_begin(); itr != edge_end(); ++itr) {
        for (auto jtr = rhs.edge_begin(); jtr != rhs.edge_end(); ++jtr) {
            (**itr).attempt_attachment_to(*jtr);
            (**jtr).attempt_attachment_to(*itr);
        }
    }
#   endif
}

void RegionEdgeLinksContainer::glue_to
    (RegionSide this_regions_side, RegionEdgeLinksContainer & other)
{
    auto other_side_idx = static_cast<int>
        (RegionSideAddress::other_side_of(this_regions_side));
    m_edges[static_cast<int>(this_regions_side)].
        glue_to(other.m_edges[other_side_idx]);
}

/* private */ RegionEdgeLink & RegionEdgeLinksContainer::edge(RegionSide side)
    { return m_edges[static_cast<int>(side)]; }

#if 0
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
#endif
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

/* static */ RegionSide RegionSideAddress::other_side_of(RegionSide side) {
    switch (side) {
    case RegionSide::bottom: return RegionSide::top   ;
    case RegionSide::left  : return RegionSide::right ;
    case RegionSide::right : return RegionSide::left  ;
    case RegionSide::top   : return RegionSide::bottom;
    }
}

int RegionSideAddress::compare(const RegionSideAddress & rhs) const {
    if (side() != rhs.side())
        { return static_cast<int>(side()) - static_cast<int>(rhs.side()); }
    return value() - rhs.value();
}

/* private */ RegionSide RegionSideAddress::other_side() const
    { return other_side_of(m_side); }

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

/* static */ bool RegionEdgeConnectionEntry::bubbleful
    (const RegionEdgeConnectionEntry & entry)
    { return entry.is_bubble(); }

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

/* static */ RegionEdgeConnectionEntry::Container
    RegionEdgeConnectionEntry::verify_no_bubbles
    (const char * constructor_name, Container && container)
{
    using namespace cul::exceptions_abbr;
    if (!std::any_of(container.begin(), container.end(),
                     RegionEdgeConnectionEntry::bubbleful))
    { return std::move(container); }
    throw InvArg{std::string{constructor_name} + "::" +
                 std::string{constructor_name} + ": all entries must point to "
                 "an existing container"};
}

/* static */ RegionEdgeConnectionEntry::Container
    RegionEdgeConnectionEntry::verify_invariants
    (const char * constructor_name, Container && container)
{
    return verify_no_bubbles
        (constructor_name,
         verify_sorted(constructor_name, std::move(container)));
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
    m_entries(RegionEdgeConnectionEntry::verify_invariants
        ("RegionEdgeConnectionsAdder", std::move(container))
    ),
    m_old_size(m_entries.size()) {}

void RegionEdgeConnectionsAdder::add
    (const Vector2I & on_field_position, const ViewGridTriangle & triangle_grid)
{
    auto addresses = RegionSideAddress::addresses_for
        (on_field_position, triangle_grid.size2());
    for (const auto & address : addresses) {
        m_entries.emplace_back
            (address,
             make_shared<RegionEdgeLinksContainer>(triangle_grid));
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
    m_entries(RegionEdgeConnectionEntry::verify_invariants
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
    auto new_end = std::remove_if
        (m_entries.begin(), m_entries.end(), has_no_link_container);
    m_entries.erase(new_end, m_entries.end());
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
    m_entries(RegionEdgeConnectionEntry::verify_invariants
        ("RegionEdgeConnectionsContainer", std::move(container))
    ) {}

RegionEdgeConnectionsAdder RegionEdgeConnectionsContainer::make_adder()
    { return RegionEdgeConnectionsAdder{std::move(m_entries)}; }

RegionEdgeConnectionsRemover RegionEdgeConnectionsContainer::make_remover()
    { return RegionEdgeConnectionsRemover{std::move(m_entries)}; }
