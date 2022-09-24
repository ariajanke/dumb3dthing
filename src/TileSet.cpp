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
#include "Texture.hpp"
#include "RenderModel.hpp"
#include "tiled-map-loader.hpp"

#include "WallTileFactory.hpp"
#include "TileFactory.hpp"

// can always clean up embedded testing later
#include <common/TestSuite.hpp>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using ConstTileSetPtr = TileSet::ConstTileSetPtr;
using TileSetPtr      = TileSet::TileSetPtr;
using Triangle        = TriangleSegment;

} // end of <anonymous> namespace

// there may, or may not be a factory for a particular id
TileFactory * TileSet::operator () (int tid) const {
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

void TileSet::load_information(Platform::ForLoaders & platform,
                               const tinyxml2::XMLElement & tileset)
{
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

    for (auto & el : XmlRange{tileset, "tile"}) {
        int id = el.IntAttribute("id");
        auto tileset_factory = TileFactory::make_tileset_factory(el.Attribute("type"));
        if (!tileset_factory)
            continue;
        insert_factory(std::move(tileset_factory), id)
            .setup(to_ts_loc(id),
                   el.FirstChildElement("properties")->FirstChildElement("property"),
                   platform);
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

// ----------------------------------------------------------------------------

GidTidTranslator::GidTidTranslator
    (const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids)
{
    if (tilesets.size() != startgids.size()) {
        throw RtError("Bug in library, GidTidTranslator constructor expects "
                      "both passed containers to be equal in size.");
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
