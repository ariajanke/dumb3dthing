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
#include "ScaleComputation.hpp"
#if 0
#include <rigtorp/HashMap.h>
#endif
#include <ariajanke/cul/HashMap.hpp>

class RegionEdgeConnectionsContainer;

struct RegionAxisAddressHasher final {
    std::size_t operator () (const RegionAxisAddress & addr) const
        { return addr.hash(); }
};

class RegionEdgeConnectionsContainerBase {
public:
    using EntryContainer = cul::HashMap
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
             const ScaledTriangleViewGrid & triangle_grid);

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
                       const ScaledTriangleViewGrid & triangle_grid);

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
