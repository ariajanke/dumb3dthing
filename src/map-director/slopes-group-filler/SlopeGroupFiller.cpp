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
using SpecialTypeFuncMap = SlopeGroupFiller::SpecialTypeFuncMap;

} // end of <anonymous> namespace

ProducableSlopeTile::ProducableSlopeTile
    (const Vector2I & map_position,
     const TileFactoryGridPtr & factory_map_layer):
    m_map_position(map_position),
    m_factory_map_layer(factory_map_layer)
{}

void ProducableSlopeTile::operator ()
    (const Vector2I & maps_offset, EntityAndTrianglesAdder & adder,
     Platform & platform) const
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
    SlopeGroupNeighborhood ninfo{intf_impl, m_map_position,maps_offset};

    auto factory = (*m_factory_map_layer)(m_map_position);
    (*factory)(adder, ninfo, platform);
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
    auto mapwide_factory_grid = make_factory_grid_for_map(tile_locations, m_tile_factories);
    UnfinishedProducableGroup<ProducableSlopeTile> group;
    for (auto & location : tile_locations) {
        group.at_location(location.on_map).
            make_producable(location.on_map, mapwide_factory_grid);
    }
    group_grid.add_group(std::move(group));
    return std::move(group_grid);
}

void SlopeGroupFiller::load
    (const TileSetXmlGrid & xml_grid, Platform & platform,
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
        auto jtr = special_type_funcs().find(tile_type);
        if (jtr != special_type_funcs().end()) {
            (this->*jtr->second)(xml_grid, r);
        }
    }

    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        auto & factory = m_tile_factories(r);
        if (!factory) { continue; }
        factory->setup(xml_grid, platform, r);
        auto * wall_factory = dynamic_cast<WallTileFactoryBase *>(factory.get());
        auto wall_tx_itr = m_pure_textures.find("wall");
        if (wall_factory && wall_tx_itr != m_pure_textures.end()) {
            wall_factory->assign_wall_texture(wall_tx_itr->second);
        }
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

/* private */ void SlopeGroupFiller::setup_pure_texture
    (const TileSetXmlGrid & xml_grid, const Vector2I & r)
{
    using cul::convert_to;
    Size2 scale{xml_grid.tile_size().width  / xml_grid.texture_size().width,
                xml_grid.tile_size().height / xml_grid.texture_size().height};
    Vector2 pos{r.x*scale.width, r.y*scale.height};
    TileTexture texture{pos, pos + convert_to<Vector2>(scale)};
    xml_grid(r).for_value("assignment", [this, texture](const auto & value) {
        m_pure_textures[value] = texture;
    });
}

/* private static */ const SpecialTypeFuncMap & SlopeGroupFiller::special_type_funcs() {
    static auto s_map = [] {
        SpecialTypeFuncMap s_map;
        s_map["pure-texture"] = &SlopeGroupFiller::setup_pure_texture;
        return s_map;
    } ();
    return s_map;
}
