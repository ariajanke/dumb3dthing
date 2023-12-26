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
    (const TilesetTileTexture & tileset_tile_tx)
{
    return FlatVertexArray
        {Vertex{k_points[k_north_west_index], tileset_tile_tx.north_west()},
         Vertex{k_points[k_south_west_index], tileset_tile_tx.south_west()},
         Vertex{k_points[k_south_east_index], tileset_tile_tx.south_east()},
         Vertex{k_points[k_north_east_index], tileset_tile_tx.north_east()}};
}

/* static */ Optional<TileCornerElevations> FlatTilesetTile::read_elevation_of
    (const MapTilesetTile & tileset_tile)
{
    if (auto elv = tileset_tile.get_numeric_property<Real>("elevation")) {
        return TileCornerElevations{*elv, *elv, *elv, *elv};
    }
    return {};
}

void FlatTilesetTile::load
    (const MapTilesetTile & tileset_tile,
     const TilesetTileTexture & tileset_tile_texture,
     PlatformAssetsStrategy & platform)
{
    auto elevations = read_elevation_of(tileset_tile);
    if (!elevations) {
        throw RuntimeError("I forgor to handle elevation not being defined");
    }
    setup(tileset_tile_texture, *elevations, platform);
}

void FlatTilesetTile::setup
    (const TilesetTileTexture & tileset_tile_texture,
     const TileCornerElevations & elevations,
     PlatformAssetsStrategy & platform)
{
    auto model = platform.make_render_model();
    const auto vertices = elevate(make_vertices(tileset_tile_texture), elevations);

    model->load(vertices.begin(), vertices.end(), k_elements.begin(), k_elements.end());
    m_corner_elevations = elevations;
    m_texture_ptr = tileset_tile_texture.texture();
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
