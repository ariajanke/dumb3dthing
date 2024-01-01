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

#include "MapTileset.hpp"

#include <tinyxml2.h>

MapTilesetTile::MapTilesetTile
    (const TiXmlElement & tile_el, const MapTileset & parent)
    { load(tile_el, parent); }

void MapTilesetTile::load
    (const TiXmlElement & tile_el, const MapTileset & parent)
{
    MapElementValuesMap map;
    map.load(tile_el);
    set_map_element_values_map(std::move(map));
    m_parent = &parent;
}

const MapTileset * MapTilesetTile::parent_tileset() const {
    return m_parent;
}

const char * MapTilesetTile::type() const {
    return get_string_attribute("type");
}

int MapTilesetTile::id() const {
    return *get_numeric_attribute<int>("id");
}

// ----------------------------------------------------------------------------

void MapTilesetImage::load(const TiXmlElement & image_el) {
    m_filename = image_el.Attribute("source", nullptr);
    m_filename = m_filename ? m_filename : "";
    m_image_size.width = image_el.IntAttribute("width");
    m_image_size.height = image_el.IntAttribute("height");
}

// ----------------------------------------------------------------------------

void MapTileset::load(const DocumentOwningNode & tileset_el) {
    MapElementValuesMap map;
    map.load(*tileset_el);
    set_map_element_values_map(std::move(map));

    auto tilecount = get_numeric_attribute<int>("tilecount");
    auto columns = get_numeric_attribute<int>("columns");
    if (!tilecount || !columns) {
        throw RuntimeError{"I forgor"};
    }
    if (*columns < 1 || *tilecount < 1) {
        throw RuntimeError{"I forgor"};
    }
    m_tile_grid.set_size(*columns, *tilecount / *columns, nullptr);
    for (auto & tile_el : XmlRange{*tileset_el, "tile"}) {
        m_tiles.emplace_back(tile_el, *this);

    }
    for (const auto & tile : m_tiles) {
        auto id = tile.id();
        if (id < 0) {
            throw RuntimeError{"I forgor"};
        }
        auto offset = id;
        m_tile_grid(offset % *columns, offset / *columns) = &tile;
    }
    m_document_owner = tileset_el;
}

const MapTilesetTile * MapTileset::tile_at(const Vector2I & r) const {
    return m_tile_grid(r);
}

const MapTilesetTile * MapTileset::seek_by_id(int id) const {
    if (auto r = id_to_tile_location(id)) {
        return m_tile_grid(*r);
    }
    return nullptr;
}

Optional<Vector2I> MapTileset::id_to_tile_location(int id) const {
    if (id < 0) {
        throw RuntimeError{""};
    }
    Vector2I r{id % m_tile_grid.width(), id / m_tile_grid.width()};
    if (m_tile_grid.has_position(r))
        { return r; }
    return {};
}

Vector2I MapTileset::next(const Vector2I & r) const {
    return m_tile_grid.next(r);
}

Vector2I MapTileset::end_position() const {
    return m_tile_grid.end_position();
}

std::size_t MapTileset::tile_count() const {
    return m_tile_grid.size();
}

Size2I MapTileset::size2() const {
    return m_tile_grid.size2();
}

MapTilesetImage MapTileset::image() const {
    MapTilesetImage rv;
    if (auto * image_el = m_document_owner->FirstChildElement("image")) {
        rv.load(*image_el);
    }
    return rv;
}

