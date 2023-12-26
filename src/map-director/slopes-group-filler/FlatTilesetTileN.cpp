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

#include "FlatTilesetTileN.hpp"
#if 0
#include "../TilesetPropertiesGrid.hpp"
#endif
#include "../MapTileset.hpp"
#include "../../RenderModel.hpp"
#include "../../TriangleSegment.hpp"

namespace {

using FlatVertexArray = FlatTilesetTile::FlatVertexArray;
constexpr const std::array<unsigned, 6> k_elements =
    FlatTilesetTile::k_elements;

} // end of <anonymous> namespace

/* static */ FlatVertexArray FlatTilesetTile::elevate
    (FlatVertexArray vertices, const TileCornerElevations & elevations)
{
    vertices[k_north_east_index].position +=
        Vector{0, elevations.north_east().value_or(0.), 0};
    vertices[k_north_west_index].position +=
        Vector{0, elevations.north_west().value_or(0.), 0};
    vertices[k_south_east_index].position +=
        Vector{0, elevations.south_east().value_or(0.), 0};
    vertices[k_south_west_index].position +=
        Vector{0, elevations.south_west().value_or(0.), 0};
    return vertices;
}

/* static */ FlatVertexArray FlatTilesetTile::make_vertices
    (const Vector2I & location_on_tileset,
     const MapTilesetTile & tileset_xml)
{
    auto tx_size = tileset_xml.texture_size();
    auto tile_size = tileset_xml.tile_size();
    Vector2 tx_tl{Real(location_on_tileset.x*tile_size.width ) / Real(tx_size.width ),
                  Real(location_on_tileset.y*tile_size.height) / Real(tx_size.height)};
    auto tile_width  = tile_size.width  / tx_size.width ;
    auto tile_height = tile_size.height / tx_size.height;
    return FlatVertexArray
        {Vertex{k_points[k_north_west_index], tx_tl},
         Vertex{k_points[k_south_west_index], tx_tl + Vector2{0, tile_height}},
         Vertex{k_points[k_south_east_index], tx_tl + Vector2{tile_width, tile_height}},
         Vertex{k_points[k_north_east_index], tx_tl + Vector2{tile_width, 0}}};
}

/* static */ Optional<TileCornerElevations> FlatTilesetTile::read_elevation_of
    (const MapTilesetTile & tile_properties)
{
    if (auto elv = tile_properties.get_numeric_property<Real>("elevation")) {
        return TileCornerElevations{*elv, *elv, *elv, *elv};
    }
    return {};
}

void FlatTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const Vector2I & location_on_tileset,
     PlatformAssetsStrategy & platform)
{
    auto elevations = read_elevation_of(map_tileset_tile);
    if (!elevations) {
        throw RuntimeError("I forgor to handle elevation not being defined");
    }
    setup(tileset_xml, location_on_tileset, platform, *elevations);
}

void FlatTilesetTile::setup
    (const MapTilesetTile & tileset_xml,
     const Vector2I & location_on_tileset,
     PlatformAssetsStrategy & platform,
     const TileCornerElevations & elevations)
{
    auto model = platform.make_render_model();
    const auto vertices =
        elevate(make_vertices(location_on_tileset, tileset_xml), elevations);

    model->load(vertices.begin(), vertices.end(), k_elements.begin(), k_elements.end());
    m_corner_elevations = elevations;
    m_texture_ptr = tileset_xml.texture();
    m_render_model = model;
    m_vertices = vertices;
}

TileCornerElevations FlatTilesetTile::corner_elevations() const {
    return m_corner_elevations;
}

void FlatTilesetTile::make
    (const TileCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{
    callbacks.
        add_entity_from_tuple(TupleBuilder{}.
            add(SharedPtr<const Texture>{m_texture_ptr}).
            add(SharedPtr<const RenderModel>{m_render_model}).
            finish());
    callbacks.add_collidable(TriangleSegment
        {m_vertices[k_elements[0]].position,
         m_vertices[k_elements[1]].position,
         m_vertices[k_elements[2]].position});
    callbacks.add_collidable(TriangleSegment
        {m_vertices[k_elements[3]].position,
         m_vertices[k_elements[4]].position,
         m_vertices[k_elements[5]].position});
}
