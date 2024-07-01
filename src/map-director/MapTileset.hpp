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

#include "../Definitions.hpp"

#include "MapElementValuesMap.hpp"
#include "DocumentOwningXmlElement.hpp"

class MapTileset;

class MapTilesetTile final : public MapElementValuesAggregable {
public:
    MapTilesetTile() {}

    MapTilesetTile(const TiXmlElement &, const MapTileset & parent);

    void load(const TiXmlElement &, const MapTileset & parent);

    void load(const TiXmlElement &);

    const MapTileset * parent_tileset() const;

    const char * type() const;

    int id() const;

private:
    void load(const TiXmlElement &, const MapTileset * parent);

    const MapTileset * m_parent = nullptr;
};

// ----------------------------------------------------------------------------

class MapTilesetImage final {
public:
    MapTilesetImage() {}

    explicit MapTilesetImage(const TiXmlElement & tile_el)
        { load(tile_el); }

    void load(const TiXmlElement &);

    const char * filename() const { return m_filename; }

    Size2 image_size() const { return m_image_size; }

private:
    Size2 m_image_size;

    const char * m_filename = "";
};

// ----------------------------------------------------------------------------

class MapTileset final : public MapElementValuesAggregable {
public:
    void load(const DocumentOwningXmlElement & tileset_el);

    const MapTilesetTile * tile_at(const Vector2I &) const;

    const MapTilesetTile * seek_by_id(int id) const;

    Optional<Vector2I> id_to_tile_location(int id) const;

    Vector2I next(const Vector2I &) const;

    Vector2I end_position() const;

    std::size_t tile_count() const;

    Size2I size2() const;

    MapTilesetImage image() const;

private:
    std::vector<MapTilesetTile> m_tiles;
    Grid<const MapTilesetTile *> m_tile_grid;
    DocumentOwningXmlElement m_document_owner;
};
