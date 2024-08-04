/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "SlopesTilesetTile.hpp"
#include "../../RenderModel.hpp"

class TileProperties;
class RampPropertiesLoaderBase;

class QuadBasedTilesetTile final : public SlopesTilesetTile {
public:
    using FlatVertexArray = std::array<Vertex, 4>;
    using ElementArray = std::array<unsigned, 6>;
    enum class Orientation { nw_to_se_elements, sw_to_ne_elements, any_elements };
    class WithPropertiesLoader {
    public:
        virtual ~WithPropertiesLoader() {}

        virtual void operator() (RampPropertiesLoaderBase &) const = 0;
    };
    using RampPropertiesLoaderStrategy = void(*)(const WithPropertiesLoader &);

    static constexpr const std::size_t k_north_west_index = 0;
    static constexpr const std::size_t k_south_west_index = 1;
    static constexpr const std::size_t k_south_east_index = 2;
    static constexpr const std::size_t k_north_east_index = 3;

    static const constexpr std::array k_points = {
        k_tile_top_left, // nw
        k_tile_top_left - k_north, // sw
        k_tile_top_left - k_north + k_east, // se
        k_tile_top_left + k_east  // ne
    };

    static constexpr const ElementArray k_nw_to_se_elements =
        { 0, 1, 2, 0, 2, 3 };

    static constexpr const ElementArray k_sw_to_ne_elements =
        { 0, 1, 3, 1, 2, 3 };

    static constexpr const ElementArray k_any_quad_elements = k_nw_to_se_elements;

    static FlatVertexArray elevate
        (FlatVertexArray vertices, const TileCornerElevations & elevations);

    static FlatVertexArray make_vertices(const TilesetTileTexture &);

    static void default_ramp_properties_loader_strategy(const WithPropertiesLoader &);

    QuadBasedTilesetTile() {}

    QuadBasedTilesetTile(RampPropertiesLoaderStrategy);

    const TileCornerElevations & corner_elevations() const final;

    void make(ProducableTileCallbacks & callbacks) const;

    void make
        (const NeighborCornerElevations &,
         ProducableTileCallbacks & callbacks) const final;

    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    void setup
        (const TilesetTileTexture & tileset_tile_texture,
         const TileCornerElevations & elevations,
         PlatformAssetsStrategy & platform);

    void set_orientation(Orientation);

private:
    using TileDecoration = SlopesAssetsRetrieval::TileDecoration;
    template <typename Func>
    void with_loader(Func && f) const;

    RampPropertiesLoaderStrategy m_properties_loader_strategy =
        default_ramp_properties_loader_strategy;
    ElementArray m_elements = k_any_quad_elements;
    TileCornerElevations m_corner_elevations;
    FlatVertexArray m_vertices;
    SharedPtr<const Texture> m_texture_ptr;
    SharedPtr<const RenderModel> m_render_model;
};

// ----------------------------------------------------------------------------

class RampPropertiesLoaderBase {
public:
    using WithPropertiesLoader = QuadBasedTilesetTile::WithPropertiesLoader;
    using Orientation = QuadBasedTilesetTile::Orientation;

    static Optional<TileCornerElevations>
        read_elevation_of(const MapTilesetTile &);

    static Optional<CardinalDirection> read_direction_of
        (const MapTilesetTile &);

    virtual ~RampPropertiesLoaderBase() {}

    void load(const MapTilesetTile & tile);

    Orientation elements_orientation() const { return m_orientation; }

    const TileCornerElevations & corner_elevations() const;

protected:
    virtual TileCornerElevations elevation_offsets_for
        (CardinalDirection direction) const = 0;

    virtual Orientation orientation_for(CardinalDirection) const = 0;

private:
    Orientation m_orientation = Orientation::any_elements;
    TileCornerElevations m_elevations;
};

// ----------------------------------------------------------------------------

class FlatPropertiesLoader final : public RampPropertiesLoaderBase {
public:
    static void instantiate_for(const WithPropertiesLoader & with_loader) {
        FlatPropertiesLoader loader;
        with_loader(loader);
    }

    TileCornerElevations elevation_offsets_for
        (CardinalDirection) const final
    { return TileCornerElevations{0, 0, 0, 0}; }

    Orientation orientation_for(CardinalDirection) const final
        { return Orientation::any_elements; }
};
