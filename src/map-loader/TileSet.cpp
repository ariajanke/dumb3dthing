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
#include "../Texture.hpp"
#include "../RenderModel.hpp"

#include "WallTileFactory.hpp"
#include "TileFactory.hpp"
#include "RampTileFactory.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using ConstTileSetPtr = TileSet::ConstTileSetPtr;
using TileSetPtr      = TileSet::TileSetPtr;
using Triangle        = TriangleSegment;

const TiXmlElement * get_first_property(const TiXmlElement & el) {
    auto props = el.FirstChildElement("properties");
    return props ? props->FirstChildElement("property") : props;
}

} // end of <anonymous> namespace

void TileSetXmlGrid::load(Platform & platform, const TiXmlElement & tileset) {
    Grid<const TiXmlElement *> tile_grid;

    if (int columns = tileset.IntAttribute("columns")) {
        tile_grid.set_size
            (tileset.IntAttribute("tilecount") / columns, columns, nullptr);
    }
    m_tile_size = Size2
        {tileset.IntAttribute("tilewidth"), tileset.IntAttribute("tileheight")};

#   if 0
    int tile_width = tileset.IntAttribute("tilewidth");
    int tile_height = tileset.IntAttribute("tileheight");
    int tile_count = tileset.IntAttribute("tilecount");
    auto to_ts_loc = [&tileset] {
        int columns = tileset.IntAttribute("columns");
        return [columns] (int n) { return Vector2I{n % columns, n / columns}; };
    } ();

    auto image_el = tileset.FirstChildElement("image");
    int tx_width = image_el->IntAttribute("width");
    int tx_height = image_el->IntAttribute("height");

    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el->Attribute("source"));

    set_texture_information(tx, Size2{tile_width, tile_height}, Size2{tx_width, tx_height});
    m_tile_count = tile_count;

    TileParams tparams{Size2(tile_width, tile_height), platform};
    for (auto & el : XmlRange{tileset, "tile"}) {
        auto * type = el.Attribute("type");
        if (!type) continue;
        auto itr = tiletype_handlers().find(type);
        if (itr == tiletype_handlers().end()) continue;

        int id = el.IntAttribute("id");
        (this->*itr->second)(el, id, to_ts_loc(id), tparams);
    }
#   endif
}

void TileSetXmlGrid::load_texture(Platform & platform, const TiXmlElement & tileset) {
    auto image_el = tileset.FirstChildElement("image");
    int tx_width = image_el->IntAttribute("width");
    int tx_height = image_el->IntAttribute("height");
    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el->Attribute("source"));
}


#if 0
// there may, or may not be a factory for a particular id
Tuple<TileFactory *, TileProducableFiller *> TileSet::operator () (int tid) const {
    return make_tuple(factory_for_id(tid), &group_filler_for_id(tid));
}

TileProducableFiller & TileSet::group_filler_for_id(int tid) const {
    auto itr = m_group_factory_map.find(tid);
    if (itr == m_group_factory_map.end()) {
        throw InvArg{  "TileSet::group_filler_for_id: tid " + std::to_string(tid)
                     + " is not a valid id for this tileset"};
    }
    assert(itr->second);
    return *itr->second;
}
#endif
TileFactory * TileSet::factory_for_id(int tid) const {
    auto itr = m_factory_map.find(tid);
    if (itr == m_factory_map.end()) return nullptr;
    return itr->second.get();
}

TileFactory & TileSet::insert_factory(UniquePtr<TileFactory> uptr, int tid) {
    auto itr = m_factory_map.find(tid);
    if (itr != m_factory_map.end()) {
        throw RtError{"TileSet::insert_factory: tile id already assigned a "
                      "factory. Only one factory is permitted per id."};
    }
    auto & factory = *(m_factory_map[tid] = std::move(uptr));
    factory.set_shared_texture_information(m_texture, m_texture_size, m_tile_size);
    return factory;
}

void TileSet::load(Platform & platform, const TiXmlElement & tileset) {
    int tile_width = tileset.IntAttribute("tilewidth");
    int tile_height = tileset.IntAttribute("tileheight");
    int tile_count = tileset.IntAttribute("tilecount");
    auto to_ts_loc = [&tileset] {
        int columns = tileset.IntAttribute("columns");
        return [columns] (int n) { return Vector2I{n % columns, n / columns}; };
    } ();

    auto image_el = tileset.FirstChildElement("image");
    int tx_width = image_el->IntAttribute("width");
    int tx_height = image_el->IntAttribute("height");

    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el->Attribute("source"));

    set_texture_information(tx, Size2{tile_width, tile_height}, Size2{tx_width, tx_height});
    m_tile_count = tile_count;

    TileParams tparams{Size2(tile_width, tile_height), platform};
    for (auto & el : XmlRange{tileset, "tile"}) {
        auto * type = el.Attribute("type");
        if (!type) continue;
        auto itr = tiletype_handlers().find(type);
        if (itr == tiletype_handlers().end()) continue;

        int id = el.IntAttribute("id");
        (this->*itr->second)(el, id, to_ts_loc(id), tparams);
    }
}

void TileSet::set_texture_information
    (const SharedPtr<const Texture> & texture, const Size2 & tile_size,
     const Size2 & texture_size)
{
    m_texture = texture;
    m_texture_size = texture_size;
    m_tile_size = tile_size;
}

/* private static */ const TileSet::TileTypeFuncMap &
    TileSet::tiletype_handlers()
{
    static const TileTypeFuncMap s_map = [] {
        TileTypeFuncMap s_map;
        s_map["pure-texture"] = &TileSet::load_pure_texture;
        s_map["in-wall"     ] = &TileSet::load_usual_wall_factory<InWallTileFactory>;
        s_map["out-wall"    ] = &TileSet::load_usual_wall_factory<OutWallTileFactory>;
        s_map["wall"        ] = &TileSet::load_usual_wall_factory<TwoWayWallTileFactory>;
        s_map["in-ramp"     ] = &TileSet::load_usual_factory<InRampTileFactory>;
        s_map["out-ramp"    ] = &TileSet::load_usual_factory<OutRampTileFactory>;
        s_map["ramp"        ] = &TileSet::load_usual_factory<TwoRampTileFactory>;
        s_map["flat"        ] = &TileSet::load_usual_factory<FlatTileFactory>;
        return s_map;
    } ();
    return s_map;
}
#if 0
/* private static */ const TileSet::TileGroupFuncMap &
    TileSet::tilegroup_handlers()
{
    static const auto s_map = [] {
        TileGroupFuncMap s_map;
        auto ramp_list = {
            "in-wall", "out-wall", "wall", "in-ramp", "out-ramp", "ramp",
            "flat"
        };
        for (const auto ramp_type : ramp_list) {
#           if 0
            s_map[ramp_type] = [] (ProcessedLayer && processed_layer) -> Tuple<UniquePtr<TileGroup>, ProcessedLayer> {
                assert(processed_layer.remaining_ids > 0);
                // ids of all like type, type strings are lost at this point...
            };
#           endif
        }
        return s_map;
    } ();
    return s_map;
}
#endif
/* private */ void TileSet::load_pure_texture
    (const TiXmlElement & el, int, Vector2I r, TileParams &)
{
    TiXmlIter itr{get_first_property(el), "property"};
    itr = std::find_if(itr, TiXmlIter{}, [](const TiXmlElement & el) {
        auto attr = el.Attribute("name");
        return attr ? std::strcmp(attr, "assignment") == 0 : false;
    });
    if (!(itr != TiXmlIter{})) return;

    auto * assignment = (*itr).Attribute("value");
    if (!assignment) return;
    using cul::convert_to;
    Size2 scale{m_tile_size.width  / m_texture_size.width ,
                m_tile_size.height / m_texture_size.height};
    Vector2 pos{r.x*scale.width, r.y*scale.height};
    m_tile_texture_map[assignment] =
        TileTexture{pos, pos + convert_to<Vector2>(scale)};
}

/* private */ void TileSet::load_factory
    (const TiXmlElement & el, UniquePtr<TileFactory> factory,
     int id, Vector2I r, Platform & platform)
{
    if (!factory)
        return;
    insert_factory(std::move(factory), id)
        .setup(r, get_first_property(el), platform);
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
    ProducableRampTile
        (const Vector2I & map_position,
         const SharedPtr<SharedPtr<Grid<TileFactory *>>> & factory_map_layer):
        m_map_position(map_position),
        m_factory_map_layer(factory_map_layer)
    {}

    void operator ()
        (const Vector2I & maps_offset, EntityAndTrianglesAdder & adder,
         Platform & platform) const final
    {
#       if 0 // yay, time for another layer of indirection!
        class Impl final : public SlopesGridInterface {
        public:
            explicit Impl( );

            Slopes operator () (Vector2I) const;
        };
        Impl intf_impl{m_slopes_grid};
        NeighborInfo ninfo{intf_impl, m_map_position,maps_offset};


        (*(*m_slopes_grid)(m_map_position))(adder, ninfo, platform);
#       endif
    }

private:
    Vector2I m_map_position;

    SharedPtr<SharedPtr<Grid<TileFactory *>>> m_factory_map_layer;
};
#if 0
class RampGroupFiller final : public TileProducableFiller {
    UnfinishedTileGroupGrid operator ()
        (const std::vector<Vector2I> & positions,
         UnfinishedTileGroupGrid && group_grid) const final
    {
        auto factory_grid = make_shared<Grid<TileFactory *>>();
        // class SlopesGridInterface... needed for neighbor info
        // I can know specific factory types (all "ramps")
        auto ramp_tile_group = group_grid.start_group_for_type<ProducableRampTile>();
        for (auto r : positions) {
#           if 0
            // r is... a position in the map
            // so I need one more parameter for "producable", that is an offset
            ramp_tile_group->at_position(r).make_producable(/* nab things */);
#           endif
        }
        return std::move(group_grid);
    }
};
#endif

class RampGroupFiller final : public TileProducableFiller {
public:

    static SharedPtr<Grid<TileFactory *>> make_factory_grid_for_map(const std::vector<TileLocation> & tile_locations, const Grid<UniquePtr<TileFactory>> & tile_factories);

    UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> & tile_locations,
         UnfinishedTileGroupGrid && group_grid) const final
    {
        auto mapwide_factory_grid = make_factory_grid_for_map(tile_locations, m_tile_factories);
        auto group = group_grid.start_group_for_type<ProducableRampTile>();
        for (auto r : tile_locations) {
            group->at_position(r.location_on_map).
                make_producable(r.location_on_map, mapwide_factory_grid);
        }
        return std::move(group_grid);
    }

    void load(const TileSetXmlGrid & xml_grid) {
        // I know how large the grid should be

        m_tile_factories.set_size(xml_grid.size2(), nullptr);
        for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
            const auto & el = xml_grid(r);

            // I know the specific tile factory type
            ;

        }
    }

private:
    Grid<UniquePtr<TileFactory>> m_tile_factories; // <- right here for tileset factory stuff...
};

static SharedPtr<TileProducableFiller> make_ramp_group_filler
        (const TileSetXmlGrid & xml_grid)
    {

        auto rv = make_shared<RampGroupFiller>();
        rv->load(xml_grid);
        return rv;
    }
