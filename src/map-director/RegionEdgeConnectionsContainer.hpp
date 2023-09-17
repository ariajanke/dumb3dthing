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

#include "RegionAxisLinksContainer.hpp"

#include <rigtorp/HashMap.h>

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

struct RegionAxisAddressHasher final {
    std::size_t operator () (const RegionAxisAddress & addr) const
        { return addr.hash(); }
};

class RegionEdgeConnectionsContainerBase {
public:
    using EntryContainer = rigtorp::HashMap
        <RegionAxisAddress,
         Variant<RegionAxisLinksContainer,
                 RegionAxisLinksAdder,
                 RegionAxisLinksRemover>,
         RegionAxisAddressHasher>;

protected:
    RegionEdgeConnectionsContainerBase() {}

    static const EntryContainer s_default_entry_container;
};

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
