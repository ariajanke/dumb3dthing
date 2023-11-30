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

template <typename T>
UniquePtr<SlopesBasedTileFactory> make_unique_base_factory() {
    static_assert(std::is_base_of_v<SlopesBasedTileFactory, T>);
    return make_unique<T>();
}

} // end of <anonymous> namespace

ProducableSlopeTile::ProducableSlopeTile
    (const Vector2I & map_position,
     const TileFactoryGridPtr & factory_map_layer):
    m_map_position(map_position),
    m_factory_map_layer(factory_map_layer)
{}

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

/* static */ SharedPtr<Grid<SlopesBasedTileFactory *>>
    SlopeGroupFiller::make_factory_grid_for_map
    (const std::vector<TileLocation> & tile_locations,
     const Grid<SharedPtr<SlopesBasedTileFactory>> & tile_factories)
{
    // I don't know how big things are... so I have to figure it out
    // This will need to change to ensure that the grid is large enough to
    // subgrid creation

    Size2I map_grid_size = [&tile_locations] {
        static constexpr const auto k_min = std::numeric_limits<int>::min();
        Size2I res{k_min, k_min};
        for (auto & tl : tile_locations) {
            res.width = std::max(tl.on_map.x + 1, res.width);
            res.height = std::max(tl.on_map.y + 1, res.height);
        }
        return res;
    } ();

    auto factory_grid_for_map = make_shared<Grid<SlopesBasedTileFactory *>>();
    factory_grid_for_map->set_size(map_grid_size, nullptr);
    for (auto & location : tile_locations) {
        (*factory_grid_for_map)(location.on_map) =
            tile_factories(location.on_tileset).get();
    }
    return factory_grid_for_map;
}

ProducableGroupTileLayer SlopeGroupFiller::operator ()
    (const std::vector<TileLocation> & tile_locations,
     ProducableGroupTileLayer && group_grid) const
{
    class SpecialLittleOwner final : public ProducableGroup_ {
    public:
        SpecialLittleOwner(const TileFactoryGrid & grid):
            factory_grid(std::move(grid)) {}

        TileFactoryGrid factory_grid;
    };

    auto factory_owner = make_shared<SpecialLittleOwner>(m_tile_factories);
    auto mapwide_factory_grid = make_factory_grid_for_map
        (tile_locations, factory_owner->factory_grid /* m_tile_factories */);
    UnfinishedProducableGroup<ProducableSlopeTile> group;
    for (auto & location : tile_locations) {
        group.
            at_location(location.on_map).
            make_producable(location.on_map, mapwide_factory_grid);
    }
    group_grid.add_group(std::move(group));
    group_grid.add_group(std::move(factory_owner));
    return std::move(group_grid);
}
#if 0
class Impl final : public ProducableGroupFiller::ProducableGroupCreation {
public:
    using TileFactoryGrid = Grid<SharedPtr<SlopesBasedTileFactory>>;

    void copy_factory_grid(const TileFactoryGrid & factory_grid) {
        owner = make_shared<SpecialLittleOwner>();
        owner->factory_grid = factory_grid;
    }

    void set_mapwide_factory_grid(const SharedPtr<Grid<SlopesBasedTileFactory *>> & factgrid)
        { m_mapwide_factory_grid = factgrid; }

    void reserve(std::size_t number_of_members) final {
        assert(owner);
        owner->tiles.reserve(number_of_members);
    }

    void add_member(const TileLocation & tile_loc) final {
        assert(owner);
        owner->tiles.emplace_back(ProducableSlopeTile{tile_loc.on_map, m_mapwide_factory_grid});
    }

    SharedPtr<ProducableGroup_> finish() final {
        return owner;
    }

private:
    class SpecialLittleOwner final : public ProducableGroup_ {
    public:
        SpecialLittleOwner(const TileFactoryGrid & grid):
            factory_grid(std::move(grid)) {}

        TileFactoryGrid factory_grid;
        std::vector<ProducableSlopeTile> tiles;
    };

    SharedPtr<Grid<SlopesBasedTileFactory *>> m_mapwide_factory_grid;
    SharedPtr<SpecialLittleOwner> owner;
};

void SlopeGroupFiller::make_group
    (CallbackWithCreator & callback) const
{

    Impl impl;
    impl.copy_factory_grid(m_tile_factories);
    auto mapwide_factory_grid = make_factory_grid_for_map
        (tile_locations, factory_owner->factory_grid /* m_tile_factories */);
    impl.set_mapwide_factory_grid()
    callback(impl);
}
#endif
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
    // I know how large the grid should be
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
