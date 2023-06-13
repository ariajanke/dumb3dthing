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

#pragma once

#include "../Defs.hpp"
#include "MapRegion.hpp"

enum class RegionSide { left, right, bottom, top };

class RegionSideAddress final {
public:
    static std::array<RegionSideAddress, 4> addresses_for
        (const Vector2I & on_field_position, const Size2I & grid_size);

    static RegionSide other_side_of(RegionSide);

    RegionSideAddress() {}

    RegionSideAddress(RegionSide side_, int value_):
        m_side(side_), m_value(value_) {}

    RegionSide side() const { return m_side; }

    int value() const { return m_value; }

    RegionSideAddress to_other_side() const
        { return RegionSideAddress{other_side(), m_value}; }

    bool operator == (const RegionSideAddress & rhs) const
        { return compare(rhs) == 0; }

    // I want space ship operator :c

    int compare(const RegionSideAddress & rhs) const;

private:
    RegionSide other_side() const;

    RegionSide m_side;
    int m_value;
};

class RegionEdgeLinkPrivate {
    friend class RegionEdgeLink;
    friend class RegionEdgeLinksContainer;
    using LinkContainerImpl = std::vector<SharedPtr<TriangleLink>>;
};

class RegionEdgeLink final {
    using LinkContainerImpl = RegionEdgeLinkPrivate::LinkContainerImpl;
public:
    RegionEdgeLink() {}

    RegionEdgeLink(LinkContainerImpl::iterator begin_,
                   LinkContainerImpl::iterator end_):
        m_begin(begin_),
        m_end(end_) {}

    // O(n^2)
    void glue_to(RegionEdgeLink &);

private:
    LinkContainerImpl::iterator m_begin;
    LinkContainerImpl::iterator m_end;
};

/// container of triangle links, used to glue segment triangles together
class RegionEdgeLinksContainer final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionEdgeLinksContainer() {}

    explicit RegionEdgeLinksContainer(const ViewGridTriangle & views);

    void glue_to(RegionEdgeLinksContainer & rhs);

    void glue_to(RegionSide this_regions_side, RegionEdgeLinksContainer &);

private:
#   if 0
    using LinkContainerImpl = RegionEdgeLinkPrivate::LinkContainerImpl;
    using Iterator = ViewGridTriangle::ElementIterator;

    static bool is_edge_tile(const ViewGridTriangle & grid, const Vector2I & r);

    static bool is_not_edge_tile(const ViewGridTriangle & grid, const Vector2I & r);

    template <bool (*meets_pred)(const ViewGridTriangle &, const Vector2I &)>
    static void append_links_by_predicate
        (const ViewGridTriangle & views, std::vector<SharedPtr<TriangleLink>> & links);

    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    std::vector<SharedPtr<TriangleLink>> m_links;
    Iterator m_edge_begin;
#   endif
    RegionEdgeLink & edge(RegionSide);

    std::vector<SharedPtr<TriangleLink>> m_links;
    std::array<RegionEdgeLink, 4> m_edges;
};

struct RegionSideAddressHasher final {
    std::size_t operator () (const RegionSideAddress &) const;
};


class RegionEdgeConnectionEntry final {
public:
    using Container = std::vector<RegionEdgeConnectionEntry>;
    using Iterator = Container::iterator;

    static bool less_than(const RegionEdgeConnectionEntry & lhs,
                          const RegionEdgeConnectionEntry & rhs);

    static bool bubbleful(const RegionEdgeConnectionEntry &);

    static Container verify_sorted
        (const char * constructor_name, Container && container);

    static Container verify_no_bubbles
        (const char * constructor_name, Container && container);

    static Container verify_invariants
        (const char * constructor_name, Container && container);

    static RegionEdgeConnectionEntry * seek
        (const RegionSideAddress & addr, Iterator begin, Iterator end);

    RegionEdgeConnectionEntry() {}

    RegionEdgeConnectionEntry(RegionSideAddress,
                              const SharedPtr<RegionEdgeLinksContainer> &);

    RegionSideAddress address() const { return m_address; }

    const SharedPtr<RegionEdgeLinksContainer> & link_container() const
        { return m_link_container; }

    bool is_bubble() const { return !!m_link_container; }

private:
    RegionSideAddress m_address;
    SharedPtr<RegionEdgeLinksContainer> m_link_container;
};

class RegionEdgeConnectionsContainer;

class RegionEdgeConnectionsAdder final {
public:
    using ViewGridTriangle = RegionEdgeLinksContainer::ViewGridTriangle;

    RegionEdgeConnectionsAdder() {}

    explicit RegionEdgeConnectionsAdder
        (RegionEdgeConnectionEntry::Container &&);

    void add(const Vector2I & on_field_position,
             const ViewGridTriangle & triangle_grid);

    RegionEdgeConnectionsContainer finish();

private:
    using Iterator = RegionEdgeConnectionEntry::Container::iterator;

    RegionEdgeConnectionEntry::Container m_entries;
    std::size_t m_old_size = 0;
};

class RegionEdgeConnectionsRemover final {
public:
    explicit RegionEdgeConnectionsRemover
        (RegionEdgeConnectionEntry::Container &&);

    void remove_region(const Vector2I & on_field_position,
                       const Size2I & grid_size);

    RegionEdgeConnectionsContainer finish();

private:
    using Iterator = RegionEdgeConnectionEntry::Container::iterator;

    static bool has_no_link_container(const RegionEdgeConnectionEntry & entry)
        { return !!entry.link_container(); }

    RegionEdgeConnectionEntry * seek(const RegionSideAddress &);

    RegionEdgeConnectionEntry::Container m_entries;
};

// RegionEdgeConnectionsContainer
//
class RegionEdgeConnectionsContainer final {
public:
    // testings thoughts:
    // - smallest possibles (1x1 grids)
    // - two triangle tiles
    // - test that things link up
    using ViewGridTriangle = RegionEdgeLinksContainer::ViewGridTriangle;

    RegionEdgeConnectionsContainer() {}

    explicit RegionEdgeConnectionsContainer
        (RegionEdgeConnectionEntry::Container &&);

    RegionEdgeConnectionsAdder make_adder();

    RegionEdgeConnectionsRemover make_remover();

private:
    RegionEdgeConnectionEntry::Container m_entries;
};
