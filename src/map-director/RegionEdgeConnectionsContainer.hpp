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
#include "../TriangleLink.hpp"

#include "MapRegion.hpp"

#include <rigtorp/HashMap.h>

#include <iostream>

enum class RegionSide {
    left  , // west
    right , // east
    bottom, // south
    top   , // north
    uninitialized
};

class RegionAxisLinksAdder;
class RegionAxisLinksRemover;

enum class RegionAxis { x_ways, z_ways, uninitialized };

RegionAxis side_to_axis(RegionSide);

template <typename Func>
void for_each_tile_on_edge
    (const RectangleI & bounds, RegionSide side, Func && f);

template <typename T, typename Func>
void for_each_tile_on_edge
    (const ViewGrid<T> & view_grid, RegionSide side, Func && f);

class RegionAxisLinkEntry final {
public:
    static bool bounds_less_than(const RegionAxisLinkEntry & lhs,
                                 const RegionAxisLinkEntry & rhs);

    static bool pointer_less_than(const RegionAxisLinkEntry & lhs,
                                  const RegionAxisLinkEntry & rhs);

    static bool pointer_equal(const RegionAxisLinkEntry & lhs,
                              const RegionAxisLinkEntry & rhs);

    static bool linkless(const RegionAxisLinkEntry & entry);

    static void attach_matching_points(const RegionAxisLinkEntry & lhs,
                                       const RegionAxisLinkEntry & rhs);

    static RegionAxisLinkEntry computed_bounds
        (const SharedPtr<TriangleLink> & link_ptr, RegionAxis axis);

    RegionAxisLinkEntry() {}

    explicit RegionAxisLinkEntry(const SharedPtr<TriangleLink> & link_ptr):
        m_link_ptr(link_ptr) {}

    RegionAxisLinkEntry
        (Real low_, Real high_, const SharedPtr<TriangleLink> & link_ptr);

    // sortable by bounds
    Real low_bounds() const { return m_low; }

    Real high_bounds() const { return m_high; }
    // sortable by link pointer
    const SharedPtr<TriangleLink> & link() const { return m_link_ptr; }

private:
    template <Real (*get_i)(const Vector &)>
    static RegionAxisLinkEntry computed_bounds_(const SharedPtr<TriangleLink> &);

    Real m_low = -k_inf;
    Real m_high = k_inf;
    SharedPtr<TriangleLink> m_link_ptr;
};

// run away effect present
class RegionAxisLinksContainer final {
public:
    RegionAxisLinksContainer() {}

    RegionAxisLinksContainer(std::vector<RegionAxisLinkEntry> && entries_,
                             RegionAxis axis_);
    // assumes no entry has a null link <- do I even need to verify for general container?

    RegionAxisLinksAdder make_adder(RegionAxis);

    RegionAxisLinksAdder make_adder();

    RegionAxisLinksRemover make_remover();

private:
    std::vector<RegionAxisLinkEntry> m_entries;
    RegionAxis m_axis = RegionAxis::uninitialized;
};

class RegionAxisLinksAdder final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    static void add_sides_from
        (RegionSide side,
         const ViewGridTriangle & triangle_grid,
         RegionAxisLinksAdder & adder);

    static std::vector<RegionAxisLinkEntry> dedupelicate
        (std::vector<RegionAxisLinkEntry> &&);

    static std::vector<RegionAxisLinkEntry> sort_and_sweep
        (std::vector<RegionAxisLinkEntry> &&);

    RegionAxisLinksAdder() {}

    RegionAxisLinksAdder
        (std::vector<RegionAxisLinkEntry> && entries_,
         RegionAxis axis_);

    // address by the adder on an axis...
    // computes bounds as you add
    // contains no dupelicate link (by pointer)
    void add(const SharedPtr<TriangleLink> & link_ptr);

    // sort (by bound) and sweep to do linking/gluing
    RegionAxisLinksContainer finish();

private:
    static std::vector<RegionAxisLinkEntry> verify_entries
        (std::vector<RegionAxisLinkEntry> &&);

    RegionAxis m_axis = RegionAxis::uninitialized;
    std::vector<RegionAxisLinkEntry> m_entries;
};

// need better name, too verbose/confusing
class RegionAxisLinksRemover final {
public:
    static std::vector<RegionAxisLinkEntry> null_out_dupelicates
        (std::vector<RegionAxisLinkEntry> &&);

    static std::vector<RegionAxisLinkEntry> remove_nulls
        (std::vector<RegionAxisLinkEntry> &&);

    RegionAxisLinksRemover() {}

    RegionAxisLinksRemover
        (std::vector<RegionAxisLinkEntry> && entries_,
         RegionAxis axis_);

    // allow dupelicate link (by pointer)
    void add(const SharedPtr<TriangleLink> & link_ptr);

    // sort by address (grab raw pointers :S)
    // eliminate on the basis if not unique
    RegionAxisLinksContainer finish();

private:
    using Entry = RegionAxisLinkEntry;

    static constexpr const bool k_verify_removals_exist_in_container = true;

    static std::vector<RegionAxisLinkEntry> verify_entries
        (std::vector<RegionAxisLinkEntry> &&);

    void verify_in_entries(const SharedPtr<TriangleLink> &);

    RegionAxis m_axis = RegionAxis::uninitialized;
    std::vector<RegionAxisLinkEntry> m_entries;
    std::size_t m_original_size = 0;
};

class RegionEdgeConnectionsContainer;

class RegionAxisAddress final {
public:
    RegionAxisAddress() {}

    RegionAxisAddress(RegionAxis axis_, int value_):
        m_axis(axis_), m_value(value_) {}

    bool operator < (const RegionAxisAddress &) const;

    bool operator == (const RegionAxisAddress &) const;

    RegionAxis axis() const { return m_axis; }

    // I want space ship :c
    int compare(const RegionAxisAddress &) const;

    int value() const { return m_value; }

    /* new */ std::size_t hash() const;

private:
    RegionAxis m_axis = RegionAxis::uninitialized;
    int m_value = 0;
};

struct RegionAxisAddressHasher_New final {
    std::size_t operator () (const RegionAxisAddress & addr) const
        { return addr.hash(); }
};

class RegionAxisAddressAndSide final {
public:
    static std::array<RegionAxisAddressAndSide, 4> for_
        (const Vector2I & on_field, const Size2I & grid_size);

    RegionAxisAddressAndSide() {}

    RegionAxisAddressAndSide(RegionAxis axis_, int value_, RegionSide side_):
        m_address(axis_, value_), m_side(side_) {}

    RegionAxisAddress address() const { return m_address; }

    RegionSide side() const { return m_side; }

    bool operator == (const RegionAxisAddressAndSide &) const;

private:
    RegionAxisAddress m_address;
    RegionSide m_side;
};
#if 0
// useful for additions and removals
class RegionEdgeConnectionChangeEntry final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionEdgeConnectionChangeEntry() {}

    RegionEdgeConnectionChangeEntry
        (RegionSide side_,
         const RegionAxisAddress & address,
         const SharedPtr<ViewGridTriangle> & triangle_grid):
        m_grid_side(side_),
        m_address(address),
        m_triangle_grid(triangle_grid) {}

    RegionSide grid_side() const { return m_grid_side; }

    RegionAxisAddress address() const { return m_address; }

    const SharedPtr<ViewGridTriangle> & triangle_grid() const
        { return m_triangle_grid; }

private:
    RegionSide m_grid_side = RegionSide::uninitialized;
    RegionAxisAddress m_address;
    SharedPtr<ViewGridTriangle> m_triangle_grid;
};
#endif
#if 0
class RegionEdgeConnectionEntry final {
public:
    using Container = std::vector<RegionEdgeConnectionEntry>;
    using Iterator = Container::iterator;

    static bool less_than(const RegionEdgeConnectionEntry & lhs,
                          const RegionEdgeConnectionEntry & rhs);

    static RegionEdgeConnectionEntry * seek
        (const RegionAxisAddress & saught, Iterator begin, Iterator end);

    // this might not be the best OOP, but I'm really not wanting to reallocate
    // things if it can be easilyish avoided

    RegionEdgeConnectionEntry() {}

    static RegionEdgeConnectionEntry start_as_adder
        (RegionAxisAddress address_, RegionAxis axis_);

    RegionAxisLinksAdder * as_adder();

    RegionAxisLinksRemover * as_remover();

    void set_container(RegionAxisLinksContainer &&);

    void reset_as_adder();

    void reset_as_remover();

    RegionAxisAddress address() const { return m_address; }

private:
    RegionEdgeConnectionEntry(cul::TypeTag<RegionAxisLinksAdder>,
                              RegionAxisAddress,
                              RegionAxis);

    RegionAxisAddress m_address;
    Variant<RegionAxisLinksContainer,
            RegionAxisLinksAdder,
            RegionAxisLinksRemover> m_axis_container = RegionAxisLinksContainer{};
};
#endif
#if 0
// nah, just use it directly, and rewrite the classes
class RegionAxisContainerWrapper_New final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    class LinkRemover final {
    public:
        LinkRemover(ViewGridTriangle & grid, RegionAxisLinksRemover & remover):
            m_grid(grid), m_remover(remover) {}

        void operator() (int x, int y) const {
            for (auto & linkptr : m_grid(x, y))
                m_remover.add(linkptr);
        }

    private:
        ViewGridTriangle & m_grid;
        RegionAxisLinksRemover & m_remover;
    };

    class LinkAdder final {
    public:
        LinkAdder(ViewGridTriangle & grid, RegionAxisLinksAdder & adder):
            m_grid(grid), m_adder(adder) {}

        void operator() (int x, int y) const {
            for (auto & linkptr : m_grid(x, y))
                m_adder.add(linkptr);
        }

    private:
        ViewGridTriangle & m_grid;
        RegionAxisLinksAdder & m_adder;
    };

    // MitM??
    RegionAxisContainerWrapper_New();

    void remove_links(const RegionAxisAddressAndSide & addr_side,
                      ViewGridTriangle & grid)
    {
        auto * remover = find_remover(addr_side.address());
        if (!remover) return;

        for_each_tile_on_edge
            (grid, addr_side.side(), LinkRemover{grid, *remover});
    }

    void add_links(const RegionAxisAddressAndSide & addr_side,
                   ViewGridTriangle & grid)
    {
        auto & adder = ensure_adder(addr_side.address());
        for_each_tile_on_edge
            (grid, addr_side.side(), LinkAdder{grid, adder});
    }

    bool are_all_adders() const;

    bool are_all_removers() const;

    bool are_all_containers() const;



private:
    using EntryContainer_New = rigtorp::HashMap
        <RegionAxisAddress,
         Variant<RegionAxisLinksContainer,
                 RegionAxisLinksAdder,
                 RegionAxisLinksRemover>,
         RegionAxisAddressHasher_New>;

    RegionAxisLinksRemover * find_remover(const RegionAxisAddress & addr) {
        auto entry = m_entries_new.find(addr);
        if (entry == m_entries_new.end()) return nullptr;
        return std::get_if<RegionAxisLinksRemover>(&entry->second);
    }

    RegionAxisLinksAdder & ensure_adder(const RegionAxisAddress & addr) {
        auto itr = m_entries_new.ensure
            (addr,
             RegionAxisLinksAdder{std::vector<RegionAxisLinkEntry>{},
                                  addr.axis()});
        return *std::get_if<RegionAxisLinksAdder>(&itr->second);
    }

    EntryContainer_New m_entries_new;
};
#endif
class RegionEdgeConnectionsContainerBase {
public:
#   if 0
    using Entry = RegionEdgeConnectionEntry;
    using ChangeEntryContainer = std::vector<RegionEdgeConnectionChangeEntry>;
    using EntryContainer = RegionEdgeConnectionEntry::Container;
#   endif
    using EntryContainer = rigtorp::HashMap
        <RegionAxisAddress,
         Variant<RegionAxisLinksContainer,
                 RegionAxisLinksAdder,
                 RegionAxisLinksRemover>,
         RegionAxisAddressHasher_New>;

protected:
#   if 0
    static ChangeEntryContainer verify_change_entries(ChangeEntryContainer &&);
#   endif
    RegionEdgeConnectionsContainerBase() {}

    static const EntryContainer s_default_entry_container;
};
#if 0
class RegionEdgeConnectionsContainer_Old;
#endif
class RegionEdgeConnectionsAdder final :
    public RegionEdgeConnectionsContainerBase
{
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionEdgeConnectionsAdder() {}

    explicit RegionEdgeConnectionsAdder(EntryContainer &&);

    void add(const Vector2I & on_field_position,
             const SharedPtr<ViewGridTriangle> & triangle_grid);

    RegionEdgeConnectionsContainer finish();

private:
    static EntryContainer verify_all_adders
        (const char * caller, EntryContainer &&);

    RegionAxisLinksAdder & ensure_adder(const RegionAxisAddress & addr);

    EntryContainer m_entries = s_default_entry_container;
};

class RegionEdgeConnectionsRemover final :
    public RegionEdgeConnectionsContainerBase {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionEdgeConnectionsRemover() {}

    explicit RegionEdgeConnectionsRemover(EntryContainer &&);

    void remove_region(const Vector2I & on_field_position,
                       const SharedPtr<ViewGridTriangle> & triangle_grid);

    RegionEdgeConnectionsContainer finish();

private:
    static EntryContainer verify_all_removers(EntryContainer &&);

    RegionAxisLinksRemover * find_remover(const RegionAxisAddress &);

    EntryContainer m_entries = s_default_entry_container;
};

class RegionEdgeConnectionsContainer final :
    public RegionEdgeConnectionsContainerBase {
public:
    RegionEdgeConnectionsContainer() {}

    explicit RegionEdgeConnectionsContainer(EntryContainer &&);

    RegionEdgeConnectionsAdder make_adder();

    RegionEdgeConnectionsRemover make_remover();

private:
    static EntryContainer verify_containers(EntryContainer &&);

    EntryContainer m_entries = s_default_entry_container;
};
#if 0
class RegionEdgeConnectionsAdder_Old final : public RegionEdgeConnectionsContainerBase {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    static EntryContainer ensure_entries_for_changes
        (const ChangeEntryContainer &, EntryContainer &&);

    static EntryContainer apply_additions
        (const ChangeEntryContainer &, EntryContainer &&);

    static EntryContainer finish_adders(EntryContainer &&);

    RegionEdgeConnectionsAdder_Old() {}

    RegionEdgeConnectionsAdder_Old(ChangeEntryContainer &&,
                                   EntryContainer &&);

    // need a different entry type
    // four entries per add
    // we will absolutely have existing axis containers
    void add(const Vector2I & on_field_position,
             const SharedPtr<ViewGridTriangle> & triangle_grid);

    // on NewEntry: sort by "axis address"
    // [!!] how do I prevent dupelicate insertions of triangle links?
    // (this might not be a problem actually... but we can add a check to be
    //  safe)
    // for each "axis address" exists an Entry
    //
    // clear new entries, and keep the oh so useful buffer
    RegionEdgeConnectionsContainer_Old finish();

private:
    ChangeEntryContainer m_change_entries;
    EntryContainer m_entries;
};
#endif
#if 0
class RegionEdgeConnectionsRemover_Old final : public RegionEdgeConnectionsContainerBase {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    static EntryContainer apply_removals
        (const ChangeEntryContainer &, EntryContainer &&);

    static EntryContainer finish_removers(EntryContainer &&);

    RegionEdgeConnectionsRemover_Old
        (ChangeEntryContainer && change_entries_,
         EntryContainer && entries);

    void remove_region(const Vector2I & on_field_position,
                       const SharedPtr<ViewGridTriangle> & triangle_grid);

    RegionEdgeConnectionsContainer_Old finish();

private:
    ChangeEntryContainer m_change_entries;
    EntryContainer m_entries;
};
#endif
#if 0
// RegionEdgeConnectionsContainer
//
class RegionEdgeConnectionsContainer_Old final : public RegionEdgeConnectionsContainerBase {
public:
    // testings thoughts:
    // - smallest possibles (1x1 grids)
    // - two triangle tiles
    // - test that things link up
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionEdgeConnectionsContainer_Old() {}

    RegionEdgeConnectionsContainer_Old
        (ChangeEntryContainer && change_entries_,
         EntryContainer && entries);

    RegionEdgeConnectionsAdder_Old make_adder();

    RegionEdgeConnectionsRemover_Old make_remover();

private:
    ChangeEntryContainer m_change_entries;
    EntryContainer m_entries;
};
#endif
template <typename T, typename Func>
void for_each_tile_on_edge
    (const ViewGrid<T> & view_grid, RegionSide side, Func && f)
{
    for_each_tile_on_edge
        (RectangleI{Vector2I{}, view_grid.size2()}, side, std::move(f));
}

template <typename Func>
void for_each_tile_on_edge
    (const RectangleI & bounds, RegionSide side, Func && f)
{
    using Side = RegionSide;
    using cul::right_of, cul::bottom_of;

    auto for_each_horz_ = [bounds, &f] (int y_pos) {
        for (int x = bounds.left; x != right_of(bounds); ++x)
            { f(x, y_pos); }
    };

    auto for_each_vert_ = [bounds, &f] (int x_pos) {
        for (int y = bounds.top; y != bottom_of(bounds); ++y)
            { f(x_pos, y); }
    };

    switch (side) {
    case Side::left  : return for_each_vert_(bounds.left          );
    case Side::right : return for_each_vert_(right_of (bounds) - 1);
    case Side::bottom: return for_each_horz_(bottom_of(bounds) - 1);
    case Side::top   : return for_each_horz_(bounds.top           );
    default: return;
    }
}
