/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "TileSet.hpp"
#include "map-loader-helpers.hpp"
#include "../Texture.hpp"
#include "../RenderModel.hpp"

#include "WallTileFactory.hpp"
#include "TileFactory.hpp"
#include "RampTileFactory.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using ConstTileSetPtr = GidTidTranslator::ConstTileSetPtr;
using TileSetPtr      = GidTidTranslator::TileSetPtr;
using Triangle        = TriangleSegment;

const TiXmlElement * get_first_property(const TiXmlElement & el) {
    auto props = el.FirstChildElement("properties");
    return props ? props->FirstChildElement("property") : props;
}

Vector2I tid_to_tileset_location(const Size2I & sz, int tid)
    { return Vector2I{tid % sz.width, tid / sz.width}; }

template <typename T>
Vector2I tid_to_tileset_location(const Grid<T> & grid, int tid)
    { return tid_to_tileset_location(grid.size2(), tid); }

} // end of <anonymous> namespace

void TileSetXmlGrid::load(Platform & platform, const TiXmlElement & tileset) {
    Grid<TileData> tile_grid;

    if (int columns = tileset.IntAttribute("columns")) {
        tile_grid.set_size
            (columns, tileset.IntAttribute("tilecount") / columns, TileData{});
    }
    Size2 tile_size
        {tileset.IntAttribute("tilewidth"), tileset.IntAttribute("tileheight")};
    load_texture(platform, tileset);

    auto to_ts_loc = [&tile_grid] (int n)
        { return tid_to_tileset_location(tile_grid, n); };

    for (auto & el : XmlRange{tileset, "tile"}) {
        static constexpr const int k_no_id = -1;
        TileData res;
        res.id = el.IntAttribute("id", k_no_id);
        res.type = el.Attribute("type");
        auto * properties = el.FirstChildElement("properties");
        for (auto & prop : XmlRange{properties, "property"}) {
            auto name = prop.Attribute("name");
            auto val = prop.Attribute("value");
            if (!name || !val) continue;
            res.properties[name] = val;
        }
        tile_grid(to_ts_loc(res.id)) = res;
    }

    // following load_texture call, no more exceptions should be possible
    std::tie(m_texture, m_texture_size) = load_texture(platform, tileset);
    m_tile_size = tile_size;
    m_elements = std::move(tile_grid);

}

/* private static */ Tuple<SharedPtr<const Texture>, Size2>
    TileSetXmlGrid::load_texture
        (Platform & platform, const TiXmlElement & tileset)
{
    auto el_ptr = tileset.FirstChildElement("image");
    if (!el_ptr) {
        throw RtError{"TileSetXmlGrid::load_texture: no texture associated with this tileset"};
    }

    const auto & image_el = *el_ptr;
    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el.Attribute("source"));
    return make_tuple(tx, Size2
        {image_el.IntAttribute("width"), image_el.IntAttribute("height")});
}

/* static */ const TileSet::FillerFactoryMap & TileSet::builtin_fillers() {
    static auto s_map = [] {
        FillerFactoryMap s_map;
        auto ramp_group_type_list =
            { "in-wall", "out-wall", "wall", "in-ramp", "out-ramp", "ramp",
              "flat" };
        for (auto type : ramp_group_type_list) {
            s_map[type] = TileProducableFiller::make_ramp_group_filler;
        }
        return s_map;
    } ();
    return s_map;
}

void TileSet::load
    (Platform & platform, const TiXmlElement & tileset,
     const FillerFactoryMap & filler_factories)
{
    // tileset loc,

    std::vector<Tuple<Vector2I, FillerFactory>> factory_grid_positions;

    TileSetXmlGrid xml_grid;
    xml_grid.load(platform, tileset);
    {
    factory_grid_positions.reserve(xml_grid.size());
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        auto el_ptr = xml_grid(r);
        if (!el_ptr) continue;
        auto & type = el_ptr->type;
        auto itr = filler_factories.find(type);
        if (itr == filler_factories.end()) {
            // warn maybe, but don't error
            continue;
        }
        if (!itr->second) {
            throw InvArg{"TileSetN::load: no filler factory maybe nullptr"};
        }
        factory_grid_positions.emplace_back(r, itr->second);
    }
    }

    std::vector<Tuple<FillerFactory, std::vector<Vector2I>>> factory_and_locations;
    {
    using Tup = Tuple<Vector2I, FillerFactory>;
    auto comp = [](const Tup & lhs, const Tup & rhs) {
        return std::less<FillerFactory>{}
            ( std::get<FillerFactory>(lhs), std::get<FillerFactory>(rhs) );
    };
    std::sort
        (factory_grid_positions.begin(), factory_grid_positions.end(),
         comp);
    FillerFactory filler_factory = nullptr;
    for (auto [r, factory] : factory_grid_positions) {
        if (filler_factory != factory) {
            factory_and_locations.emplace_back(factory, std::vector<Vector2I>{});
            filler_factory = factory;
        }
        std::get<std::vector<Vector2I>>(factory_and_locations.back()).push_back(r);
    }
    }

    Grid<SharedPtr<TileProducableFiller>> filler_grid;
    filler_grid.set_size(xml_grid.size2(), nullptr);
    for (auto [factory, locations] : factory_and_locations) {
        auto filler = (*factory)(xml_grid, platform);
        for (auto r : locations) {
            filler_grid(r) = filler;
        }
    }
    m_filler_grid = std::move(filler_grid);
}

SharedPtr<TileProducableFiller> TileSet::find_filler(int tid) const {
    return find_filler( tile_id_to_tileset_location(tid) );
}

SharedPtr<TileProducableFiller> TileSet::find_filler(Vector2I r) const {
    return m_filler_grid(r);
}

Vector2I TileSet::tile_id_to_tileset_location(int tid) const {
    return tid_to_tileset_location(m_filler_grid, tid);
}

// ----------------------------------------------------------------------------

GidTidTranslator::GidTidTranslator
    (const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids)
{
    if (tilesets.size() != startgids.size()) {
        throw RtError{"Bug in library, GidTidTranslator constructor expects "
                      "both passed containers to be equal in size."};
    }
    m_gid_map.reserve(tilesets.size());
    for (std::size_t i = 0; i != tilesets.size(); ++i) {
        m_gid_map.emplace_back(startgids[i], tilesets[i]);
    }
    if (!startgids.empty()) {
        m_gid_end = startgids.back() + tilesets.back()->total_tile_count();
    }
    m_ptr_map.reserve(m_gid_map.size());
    for (auto & pair : m_gid_map) {
        m_ptr_map.emplace_back( pair.starting_id, pair.tileset );
    }

    std::sort(m_gid_map.begin(), m_gid_map.end(), order_by_gids);
    std::sort(m_ptr_map.begin(), m_ptr_map.end(), order_by_ptrs);
}

std::pair<int, ConstTileSetPtr> GidTidTranslator::gid_to_tid(int gid) const {
    if (gid == 0) {
        return std::make_pair(0, nullptr);
    }
    if (gid < 1 || gid >= m_gid_end) {
        throw InvArg{"Given gid is either the empty tile or not contained in "
                     "this map; translatable gids: [1 " + std::to_string(m_gid_end)
                     + ")."};
    }
    GidAndTileSetPtr samp;
    samp.starting_id = gid;
    auto itr = std::upper_bound(m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    if (itr == m_gid_map.begin()) {
        throw RtError{"Library error: GidTidTranslator said that it owned a "
                      "gid, but does not have a tileset for it."};
    }
    --itr;
    assert(gid >= itr->starting_id);
    return std::make_pair(gid - itr->starting_id, itr->tileset);
}

std::pair<int, TileSetPtr> GidTidTranslator::gid_to_tid(int gid) {
    const auto & const_this = *this;
    auto gv = const_this.gid_to_tid(gid);
    return std::make_pair(gv.first, std::const_pointer_cast<TileSet>(gv.second));
}

void GidTidTranslator::swap(GidTidTranslator & rhs) {
    m_ptr_map.swap(rhs.m_ptr_map);
    m_gid_map.swap(rhs.m_gid_map);

    std::swap(m_gid_end, rhs.m_gid_end);
}

int GidTidTranslator::tid_to_gid(int tid, ConstTileSetPtr tileset) const {
    GidAndConstTileSetPtr samp;
    samp.tileset = tileset;
    auto itr = std::lower_bound(m_ptr_map.begin(), m_ptr_map.end(), samp, order_by_ptrs);
    static constexpr const auto k_unowned_msg = "Map/layer does not own this tile set.";
    if (itr == m_ptr_map.end()) {
        throw RtError(k_unowned_msg);
    } else if (itr->tileset != tileset) {
        throw RtError(k_unowned_msg);
    }
    return tid + itr->starting_id;
}

/* private static */ bool GidTidTranslator::order_by_gids
    (const GidAndTileSetPtr & lhs, const GidAndTileSetPtr & rhs)
{ return lhs.starting_id < rhs.starting_id; }

/* private static */ bool GidTidTranslator::order_by_ptrs
    (const GidAndConstTileSetPtr & lhs, const GidAndConstTileSetPtr & rhs)
{
    const void * lptr = lhs.tileset.get();
    const void * rptr = rhs.tileset.get();
    return std::less<const void *>()(lptr, rptr);
}

// ----------------------------------------------------------------------------

class ProducableRampTile final : public ProducableTile {
public:
    using NeighborInfo = TileFactory::NeighborInfo;
    using TileFactoryGridPtr = SharedPtr<Grid<TileFactory *>>;

    ProducableRampTile
        (const Vector2I & map_position,
         const TileFactoryGridPtr & factory_map_layer):
        m_map_position(map_position),
        m_factory_map_layer(factory_map_layer)
    {}

    void operator ()
        (const Vector2I & maps_offset, EntityAndTrianglesAdder & adder,
         Platform & platform) const final
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
        NeighborInfo ninfo{intf_impl, m_map_position,maps_offset};

        auto factory = (*m_factory_map_layer)(m_map_position);
        (*factory)(adder, ninfo, platform);
    }

private:
    Vector2I m_map_position;

    TileFactoryGridPtr m_factory_map_layer;
};

class RampGroupFiller final : public TileProducableFiller {
public:
    using RampGroupFactoryMakeFunc = UniquePtr<TileFactory>(*)();
    using RampGroupFactoryMap = std::map<std::string, RampGroupFactoryMakeFunc>;
    using TileTextureMap = std::map<std::string, TileTexture>;
    using SpecialTypeFunc = void(RampGroupFiller::*)(const TileSetXmlGrid & xml_grid, const Vector2I & r);
    using SpecialTypeFuncMap = std::map<std::string, SpecialTypeFunc>;
    static SharedPtr<Grid<TileFactory *>> make_factory_grid_for_map
        (const std::vector<TileLocation> & tile_locations,
         const Grid<SharedPtr<TileFactory>> & tile_factories)
    {
        // I don't know how big things are... so I have to figure it out
        // This will need to change to ensure that the grid is large enough to
        // subgrid creation

        Size2I map_grid_size = [&tile_locations] {
            static constexpr const auto k_min = std::numeric_limits<int>::min();
            Size2I res{k_min, k_min};
            for (auto & tl : tile_locations) {
                res.width = std::max(tl.location_on_map.x + 1, res.width);
                res.height = std::max(tl.location_on_map.y + 1, res.height);
            }
            return res;
        } ();

        SharedPtr<Grid<TileFactory *>> factory_grid_for_map = make_shared<Grid<TileFactory *>>();
        factory_grid_for_map->set_size(map_grid_size, nullptr);
        for (auto & tl : tile_locations) {
            (*factory_grid_for_map)(tl.location_on_map) = tile_factories(tl.location_on_tileset).get();
        }
        return factory_grid_for_map;
    }

    UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> & tile_locations,
         UnfinishedTileGroupGrid && group_grid) const final
    {
        auto mapwide_factory_grid = make_factory_grid_for_map(tile_locations, m_tile_factories);
        UnfinishedProducableGroup<ProducableRampTile> group;
        for (auto r : tile_locations) {
            group.at_position(r.location_on_map).
                make_producable(r.location_on_map, mapwide_factory_grid);
        }
        group_grid.add_group(std::move(group));
        return std::move(group_grid);
    }

    void load
        (const TileSetXmlGrid & xml_grid, Platform & platform,
         const RampGroupFactoryMap & factory_type_map = builtin_tile_factory_maker_map())
    {
        // I know how large the grid should be
        m_tile_factories.set_size(xml_grid.size2(), UniquePtr<TileFactory>{});

        // this should be a function
        for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
            const auto * el = xml_grid(r);
            if (!el)
                { continue; }
            // I know the specific tile factory type
            auto & tile_type = el->type;
            auto itr = factory_type_map.find(tile_type);
            if (itr != factory_type_map.end()) {
                auto & factory = m_tile_factories(r) = (*itr->second)();
                factory->set_shared_texture_information
                    (xml_grid.texture(), xml_grid.texture_size(), xml_grid.tile_size());
            }
            auto jtr = special_type_funcs().find(tile_type);
            if (jtr != special_type_funcs().end()) {
                (this->*jtr->second)(xml_grid, r);
            }
        }

        for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
            auto & factory = m_tile_factories(r);
            if (!factory) { continue; }
            factory->setup(r, xml_grid(r), platform);
            factory->set_shared_texture_information
                (xml_grid.texture(), xml_grid.texture_size(), xml_grid.tile_size());
            auto * wall_factory = dynamic_cast<WallTileFactoryBase *>(factory.get());
            auto wall_tx_itr = m_pure_textures.find("wall");
            if (wall_factory && wall_tx_itr != m_pure_textures.end()) {
                wall_factory->assign_wall_texture(wall_tx_itr->second);
            }
        }
    }

    // dangler hazard while moving shit around


    static const RampGroupFactoryMap & builtin_tile_factory_maker_map() {
        static auto s_map = [] {
            RampGroupFactoryMap s_map;
            s_map["in-wall"     ] = make_unique_base_factory<InWallTileFactory>;
            s_map["out-wall"    ] = make_unique_base_factory<OutWallTileFactory>;
            s_map["wall"        ] = make_unique_base_factory<TwoWayWallTileFactory>;
            s_map["in-ramp"     ] = make_unique_base_factory<InRampTileFactory>;
            s_map["out-ramp"    ] = make_unique_base_factory<OutRampTileFactory>;
            s_map["ramp"        ] = make_unique_base_factory<TwoRampTileFactory>;
            s_map["flat"        ] = make_unique_base_factory<FlatTileFactory>;
            return s_map;
        } ();
        return s_map;
    }

private:
    template <typename T>
    static UniquePtr<TileFactory> make_unique_base_factory() {
        static_assert(std::is_base_of_v<TileFactory, T>);
        return make_unique<T>();
    }

    void setup_pure_texture
        (const TileSetXmlGrid & xml_grid, const Vector2I & r)
    {
        const auto & el = *xml_grid(r);
        auto itr = el.properties.find("assignment");
        if (itr == el.properties.end()) return;
        auto & assignment = itr->second;
        using cul::convert_to;
        Size2 scale{xml_grid.tile_size().width  / xml_grid.texture_size().width ,
                    xml_grid.tile_size().height / xml_grid.texture_size().height};
        Vector2 pos{r.x*scale.width, r.y*scale.height};
        m_pure_textures[assignment] =
            TileTexture{pos, pos + convert_to<Vector2>(scale)};
    }

    static const SpecialTypeFuncMap & special_type_funcs() {
        static auto s_map = [] {
            SpecialTypeFuncMap s_map;
            s_map["pure-texture"] = &RampGroupFiller::setup_pure_texture;
            return s_map;
        } ();
        return s_map;
    }

    TileTextureMap m_pure_textures;
    Grid<SharedPtr<TileFactory>> m_tile_factories;
};

/* static */ SharedPtr<TileProducableFiller> TileProducableFiller::make_ramp_group_filler
    (const TileSetXmlGrid & xml_grid, Platform & platform)
{
    auto rv = make_shared<RampGroupFiller>();
    rv->load(xml_grid, platform);
    return rv;
}

Tuple
    <GridView<ProducableTile *>,
     std::vector<SharedPtr<ProducableGroup_>>>
    UnfinishedProducableTileGridView::move_out_producables_and_groups()
{
    GridViewInserter<ProducableTile *> producables_inserter{m_targets.front().size2()};
    for (Vector2I r; r != m_targets.front().end_position(); r = m_targets.front().next(r)) {
        for (auto & target : m_targets) {
            if (!target(r)) continue;
            producables_inserter.push(target(r));
        }
        producables_inserter.advance();
    }
    m_targets.clear();
    return make_tuple(GridView{std::move(producables_inserter)}, std::move(m_groups));
}

UnfinishedProducableTileGridView
    TileGroupGrid::add_producables_to
    (UnfinishedProducableTileGridView && unfinished_grid_view)
{
    unfinished_grid_view.add_layer(std::move(m_target), m_groups);
    return std::move(unfinished_grid_view);
}
