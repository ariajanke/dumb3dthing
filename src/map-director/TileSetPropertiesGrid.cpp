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

#include "TileSetPropertiesGrid.hpp"
#include "TileSet.hpp"

#include "../Texture.hpp"
#include "../platform.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

void TileProperties::load(const TiXmlElement & tile_el) {
    auto id = tile_el.IntAttribute("id", k_no_id);
    auto type = tile_el.Attribute("type");
    if (!type || id == k_no_id) {
        throw InvArg{"TileProperties::load: both id and type attributes must "
                     "be defined"                                             };
    }
    m_id = id;
    m_type = type;
    auto * properties = tile_el.FirstChildElement("properties");
    for (auto & prop : XmlRange{properties, "property"}) {
        auto name = prop.Attribute("name");
        auto val = prop.Attribute("value");
        if (!name || !val) continue;
        m_properties[name] = val;
    }
}

void TileSetXmlGrid::load(Platform & platform, const TiXmlElement & tileset) {
    Grid<TileProperties> tile_grid;

    if (int columns = tileset.IntAttribute("columns")) {
        tile_grid.set_size
            (columns, tileset.IntAttribute("tilecount") / columns, TileProperties{});
    }
    Size2 tile_size
        {tileset.IntAttribute("tilewidth"), tileset.IntAttribute("tileheight")};
    load_texture(platform, tileset);

    auto to_ts_loc = [&tile_grid] (int n)
        { return TileSet::tid_to_tileset_location(tile_grid, n); };

    for (auto & el : XmlRange{tileset, "tile"}) {
        TileProperties props;
        props.load(el);
        tile_grid(to_ts_loc(props.id())) = std::move(props);
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
