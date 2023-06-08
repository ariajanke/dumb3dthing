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
#include "map-loader-helpers.hpp"

class RegionSideAddress final {
public:
    enum Side { k_left_side, k_right_side, k_bottom_side, k_top_side };

    static std::array<RegionSideAddress, 4> addresses_for
        (const Vector2I & on_field_position, const Size2I & grid_size);

    RegionSideAddress() {}

    RegionSideAddress(Side side_, int value_):
        m_side(side_), m_value(value_) {}

    Side side() const { return m_side; }

    int value() const { return m_value; }

    RegionSideAddress to_other_side() const
        { return RegionSideAddress{other_side(), m_value}; }

    bool operator == (const RegionSideAddress & rhs) const
        { return compare(rhs) == 0; }

    // I want space ship operator :c

    int compare(const RegionSideAddress & rhs) const {
        if (side() != rhs.side())
            { return static_cast<int>(side()) - static_cast<int>(rhs.side()); }
        return value() - rhs.value();
    }

private:
    Side other_side() const {
        using Rsa = RegionSideAddress;
        switch (m_side) {
        case Rsa::k_bottom_side: return Rsa::k_top_side;
        case Rsa::k_left_side  : return Rsa::k_right_side;
        case Rsa::k_right_side : return Rsa::k_left_side;
        case Rsa::k_top_side   : return Rsa::k_bottom_side;
        }
    }

    Side m_side;
    int m_value;
};

struct RegionSideAddressHasher final {
    std::size_t operator () (const RegionSideAddress & address) const {
        using IntHasher = std::hash<int>;
        using SideHasher = std::hash<RegionSideAddress::Side>;
        return IntHasher{}(address.value()) ^ SideHasher{}(address.side());
    }
};

/* static */ std::array<RegionSideAddress, 4> RegionSideAddress::addresses_for
    (const Vector2I & on_field_position, const Size2I & grid_size)
{
    auto right  = on_field_position.x + grid_size.width ;
    auto bottom = on_field_position.y + grid_size.height;
    return {
        RegionSideAddress{ k_top_side   , on_field_position.y },
        RegionSideAddress{ k_bottom_side, bottom              },
        RegionSideAddress{ k_left_side  , on_field_position.x },
        RegionSideAddress{ k_right_side , right               },
    };
}

class RegionEdgeConnectionEntry final {
public:
    using Container = std::vector<RegionEdgeConnectionEntry>;

    static bool less_than(const RegionEdgeConnectionEntry & lhs,
                          const RegionEdgeConnectionEntry & rhs)
    { return lhs.address().compare(rhs.address()) < 0; }

    static Container verify_sorted
        (const char * constructor_name, Container && container)
    {
        using namespace cul::exceptions_abbr;
        if (std::is_sorted(container.begin(), container.end(), less_than))
            { return std::move(container); }
        throw InvArg{std::string{constructor_name} + "::" +
                     std::string{constructor_name} + ": entries must be sorted"};
    }

    RegionEdgeConnectionEntry() {}

    RegionEdgeConnectionEntry(RegionSideAddress address_,
                              SharedPtr<InterTriangleLinkContainer> && link_container):
        m_address(address_),
        m_link_container(std::move(link_container)) {}

    RegionSideAddress address() const
        { return m_address; }

    const SharedPtr<InterTriangleLinkContainer> & link_container() const
        { return m_link_container; }

private:
    RegionSideAddress m_address;
    SharedPtr<InterTriangleLinkContainer> m_link_container;
};

class RegionEdgeConnectionsContainer;

class RegionEdgeConnectionsAdder final {
public:
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;

    explicit RegionEdgeConnectionsAdder
        (RegionEdgeConnectionEntry::Container && container):
        m_entries(RegionEdgeConnectionEntry::verify_sorted
            ("RegionEdgeConnectionsAdder", std::move(container))
        ),
        m_old_size(m_entries.size()) {}

    void add(const Vector2I & on_field_position,
             const ViewGridTriangle & triangle_grid)
    {
        auto addrs = RegionSideAddress::addresses_for
            (on_field_position, triangle_grid.size2());
        auto link_container_ptr =
            make_shared<InterTriangleLinkContainer>(triangle_grid);

        for (auto & addr : addrs) {
            m_entries.emplace_back(link_container_ptr, addr);
        }
    }

    RegionEdgeConnectionsContainer finish();

private:
    RegionEdgeConnectionEntry::Container m_entries;
    std::size_t m_old_size = 0;
};

class RegionEdgeConnectionsRemover final {
public:
    explicit RegionEdgeConnectionsRemover(RegionEdgeConnectionEntry::Container && container):
        m_entries(RegionEdgeConnectionEntry::verify_sorted
            ("RegionEdgeConnectionsRemover", std::move(container))
        )
    {}

    void remove_region(const Vector2I & on_field_position,
                       const Size2I & grid_size)
    {
        auto addrs = RegionSideAddress::addresses_for
            (on_field_position, grid_size);
        for (auto & addr : addrs) {
            if (auto entry = seek(addr)) {
                *entry = RegionEdgeConnectionEntry{addr, nullptr};
            }
        }
    }

    RegionEdgeConnectionsContainer finish();

private:
    static bool has_no_link_container(const RegionEdgeConnectionEntry & entry)
        { return !!entry.link_container(); }

    RegionEdgeConnectionEntry * seek(const RegionSideAddress & addr) {
        using Entry = RegionEdgeConnectionEntry;
        auto itr = std::lower_bound
            (m_entries.begin(), m_entries.end(), Entry::less_than);
        if (itr == m_entries.end()) return {};
        if (itr->address() == addr) return &*itr;
        return {};
    }

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
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;

    RegionEdgeConnectionsContainer(RegionEdgeConnectionEntry::Container && container):
        m_entries(RegionEdgeConnectionEntry::verify_sorted
            ("RegionEdgeConnectionsContainer", std::move(container))
        )
    {}

    RegionEdgeConnectionsAdder make_adder()
        { return RegionEdgeConnectionsAdder{std::move(m_entries)}; }

    RegionEdgeConnectionsRemover make_remover()
        { return RegionEdgeConnectionsRemover{std::move(m_entries)}; }

private:
    RegionEdgeConnectionEntry::Container m_entries;
};

inline RegionEdgeConnectionsContainer RegionEdgeConnectionsAdder::finish() {
    using Entry = RegionEdgeConnectionEntry;
    auto dirty_begin = m_entries.begin() + m_old_size;
    std::sort(dirty_begin, m_entries.end(), Entry::less_than);
    std::inplace_merge
        (m_entries.begin(), dirty_begin, m_entries.end(), Entry::less_than);
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}

inline RegionEdgeConnectionsContainer RegionEdgeConnectionsRemover::finish() {
    std::remove_if(m_entries.begin(), m_entries.end(), has_no_link_container);
    return RegionEdgeConnectionsContainer{std::move(m_entries)};
}
