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

#include "MapRegionContainer.hpp"

class RegionAxisLinksAdder;
class RegionAxisLinksRemover;

enum class RegionSide {
    left  , // west
    right , // east
    bottom, // south
    top   , // north
    uninitialized
};

enum class RegionAxis { x_ways, z_ways, uninitialized };

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

class RegionAxisLinksContainer final {
public:
    RegionAxisLinksContainer() {}

    RegionAxisLinksContainer(std::vector<RegionAxisLinkEntry> && entries_,
                             RegionAxis axis_);

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
    using EntryIterator = std::vector<Entry>::iterator;

    static constexpr const bool k_verify_removals_exist_in_container = true;

    static std::vector<RegionAxisLinkEntry> verify_entries
        (std::vector<RegionAxisLinkEntry> &&);

    static void clear_more_than_one_entry
        (EntryIterator begin, EntryIterator end);

    void verify_in_entries(const SharedPtr<TriangleLink> &);

    RegionAxis m_axis = RegionAxis::uninitialized;
    std::vector<RegionAxisLinkEntry> m_entries;
    std::size_t m_original_size = 0;
};

template <typename Func>
void for_each_tile_on_edge
    (const RectangleI & bounds, RegionSide side, Func && f);

template <typename T, typename Func>
void for_each_tile_on_edge
    (const ViewGrid<T> & view_grid, RegionSide side, Func && f);

// ----------------------------------------------------------------------------

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
