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

#include "SlopesTilesetTileN.hpp"
#include "../../RenderModel.hpp"

class TileProperties;

template <typename ... Types>
class TupleBuilder {
public:
    TupleBuilder() {}

    TupleBuilder(Tuple<Types...> && tuple):
        m_impl(std::move(tuple)) {}

    template <typename T>
    TupleBuilder<T, Types...> add(T && obj) && {
        return TupleBuilder<T, Types...>
            {std::tuple_cat(std::make_tuple(obj), std::move(m_impl))};
    }

    void add_to_entity(Entity & ent) &&
        { ent.add<Types...>() = std::move(m_impl); }


    Tuple<Types...> finish() &&
        { return m_impl; }

private:
    Tuple<Types...> m_impl;
};

// ----------------------------------------------------------------------------

class FlatTilesetTile final : public SlopesTilesetTile {
public:
    static constexpr const std::size_t k_north_west_index = 0;
    static constexpr const std::size_t k_south_west_index = 1;
    static constexpr const std::size_t k_south_east_index = 2;
    static constexpr const std::size_t k_north_east_index = 3;

    using FlatVertexArray = std::array<Vertex, 4>;

    static constexpr const std::array<unsigned, 6> k_elements =
        {0, 1, 2, 0, 2, 3};

    static const constexpr std::array k_points = {
        k_tile_top_left, // nw
        k_tile_top_left - k_north, // sw
        k_tile_top_left - k_north + k_east, // se
        k_tile_top_left + k_east  // ne
    };

    static FlatVertexArray elevate
        (FlatVertexArray vertices, const TileCornerElevations & elevations);

    static FlatVertexArray make_vertices
        (const Vector2I & location_on_tileset,
         const MapTilesetTile & tileset_xml);

    static Optional<TileCornerElevations>
        read_elevation_of(const MapTilesetTile &);

    void load
        (const MapTilesetTile &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform) final;

    void setup
        (const MapTilesetTile & tileset_xml,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform,
         const TileCornerElevations & elevations);

    TileCornerElevations corner_elevations() const final;

    void make
        (const TileCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;

private:
    TileCornerElevations m_corner_elevations;
    FlatVertexArray m_vertices;
    SharedPtr<const Texture> m_texture_ptr;
    SharedPtr<const RenderModel> m_render_model;
};
