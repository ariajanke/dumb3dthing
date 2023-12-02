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

#include "SlopeGroupFiller.hpp"
#include "WallTileFactory.hpp"
#include "RampTileFactory.hpp"
#include "../slopes-group-filler.hpp"

namespace {

using RampGroupFactoryMap = SlopeGroupFiller::RampGroupFactoryMap;
using TileFactoryGrid = SlopeGroupFiller::TileFactoryGrid;

template <typename T>
UniquePtr<SlopesBasedTileFactory> make_unique_base_factory() {
    static_assert(std::is_base_of_v<SlopesBasedTileFactory, T>);
    return make_unique<T>();
}

// ----------------------------------------------------------------------------

class SlopesGroupOwner final : public ProducableGroupOwner {
public:
    void reserve(std::size_t number_of_members, const Size2I & grid_size) {
        m_slope_tiles.reserve(number_of_members);
        m_tileset_to_map_mapping =
            make_shared<Grid<SlopesBasedTileFactory *>>();
        m_tileset_to_map_mapping->set_size(grid_size, nullptr);
    }

    ProducableTile & add_member(const TileLocation & tile_location) {
#       ifdef MACRO_DEBUG
        assert(m_slope_tiles.capacity() > m_slope_tiles.size());
#       endif
        m_slope_tiles.emplace_back
            (ProducableSlopeTile{tile_location.on_map, m_tileset_to_map_mapping});
        (*m_tileset_to_map_mapping)(tile_location.on_map) =
            m_tile_factory_grid(tile_location.on_tileset).get();
        return m_slope_tiles.back();
    }

    void set_tile_factory_grid(const TileFactoryGrid & grid) {
        m_tile_factory_grid = grid;
    }

private:
    SharedPtr<Grid<SlopesBasedTileFactory *>> m_tileset_to_map_mapping;
    std::vector<ProducableSlopeTile> m_slope_tiles;
    TileFactoryGrid m_tile_factory_grid;
};

// ----------------------------------------------------------------------------

class SlopesGroupCreation final :
    public ProducableGroupFiller::ProducableGroupCreation
{
public:
    void reserve(std::size_t number_of_members, const Size2I & grid_size) final {
#       ifdef MACRO_DEBUG
        assert(m_owner);
#       endif
        m_owner->reserve(number_of_members, grid_size);
    }

    ProducableTile & add_member(const TileLocation & tile_location) final {
#       ifdef MACRO_DEBUG
        assert(m_owner);
#       endif
        return m_owner->add_member(tile_location);
    }

    SharedPtr<ProducableGroupOwner> finish() final
        { return std::move(m_owner); }

    void setup_with_tile_factory_grid(const TileFactoryGrid & grid) {
        m_owner = make_shared<SlopesGroupOwner>();
        m_owner->set_tile_factory_grid(grid);
    }

private:
    SharedPtr<SlopesGroupOwner> m_owner;
};

} // end of <anonymous> namespace

ProducableSlopeTile::ProducableSlopeTile
    (const Vector2I & map_position,
     const TileFactoryGridPtr & factory_map_layer):
    m_map_position(map_position),
    m_factory_map_layer(factory_map_layer) {}

void ProducableSlopeTile::operator ()
    (ProducableTileCallbacks & callbacks) const
{
    class Impl final : public SlopesGridInterface {
    public:
        explicit Impl(const TileFactoryGridPtr & grid_ptr):
            m_grid(grid_ptr) {}

        Slopes operator () (Vector2I r) const
            { return (*m_grid)(r)->tile_elevations(); }

    private:
        const TileFactoryGridPtr & m_grid;
    };
    Impl intf_impl{m_factory_map_layer};
    SlopeGroupNeighborhood ninfo{intf_impl, m_map_position};

    auto factory = (*m_factory_map_layer)(m_map_position);
    (*factory)(ninfo, callbacks);
}

// ----------------------------------------------------------------------------

void SlopeGroupFiller::make_group(CallbackWithCreator & creator) const {
    SlopesGroupCreation impl;
    impl.setup_with_tile_factory_grid(m_tile_factories);
    creator(impl);
}

void SlopeGroupFiller::load
    (const TilesetXmlGrid & xml_grid,
     PlatformAssetsStrategy & platform,
     const RampGroupFactoryMap & factory_type_map)
{
    load_factories(xml_grid, factory_type_map);
    setup_factories(xml_grid, platform, m_tile_factories);
}

/* private */ void SlopeGroupFiller::load_factories
    (const TilesetXmlGrid & xml_grid,
     const RampGroupFactoryMap & factory_type_map)
{
    m_tile_factories.set_size(xml_grid.size2(), UniquePtr<SlopesBasedTileFactory>{});

    // this should be a function
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        const auto & el = xml_grid(r);
        if (el.is_empty())
            { continue; }
        // I know the specific tile factory type
        const auto & tile_type = el.type();
        auto itr = factory_type_map.find(tile_type);
        if (itr != factory_type_map.end()) {
            m_tile_factories(r) = (*itr->second)();
        }
        m_specials.action_by_tile_type(tile_type, xml_grid, r);
    }
}

/* private */ void SlopeGroupFiller::setup_factories
    (const TilesetXmlGrid & xml_grid,
     PlatformAssetsStrategy & platform,
     TileFactoryGrid & tile_factories) const
{
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        auto & factory = tile_factories(r);
        if (!factory) { continue; }

        factory->setup(xml_grid, platform, m_specials, r);
    }
}

/* static */ const RampGroupFactoryMap &
    SlopeGroupFiller::builtin_tile_factory_maker_map()
{
    static auto s_map = [] {
        RampGroupFactoryMap s_map;
        // TODO pull from constants defined somewhere
        using namespace slopes_group_filler_type_names;
        s_map[k_in_wall     ] = make_unique_base_factory<InWallTileFactory>;
        s_map[k_out_wall    ] = make_unique_base_factory<OutWallTileFactory>;
        s_map[k_wall        ] = make_unique_base_factory<TwoWayWallTileFactory>;
        s_map[k_in_ramp     ] = make_unique_base_factory<InRampTileFactory>;
        s_map[k_out_ramp    ] = make_unique_base_factory<OutRampTileFactory>;
        s_map[k_ramp        ] = make_unique_base_factory<TwoRampTileFactory>;
        s_map[k_flat        ] = make_unique_base_factory<FlatTileFactory>;
        return s_map;
    } ();
    return s_map;
}
