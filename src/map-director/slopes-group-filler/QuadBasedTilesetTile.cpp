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

#include "QuadBasedTilesetTile.hpp"
#include "TileDecorationCreation.hpp"

#include "../MapTileset.hpp"

#include "../../RenderModel.hpp"
#include "../../TriangleSegment.hpp"

#include <map>

namespace {

using FlatVertexArray = QuadBasedTilesetTile::FlatVertexArray;
Optional<CardinalDirection> cardinal_direction_from(const char * nullable_str);

} // end of <anonymous> namespace

/* static */ FlatVertexArray QuadBasedTilesetTile::elevate
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

/* static */ FlatVertexArray QuadBasedTilesetTile::make_vertices
    (const TilesetTileTexture & tileset_tile_tx)
{
    return FlatVertexArray
        {Vertex{k_points[k_north_west_index], tileset_tile_tx.north_west()},
         Vertex{k_points[k_south_west_index], tileset_tile_tx.south_west()},
         Vertex{k_points[k_south_east_index], tileset_tile_tx.south_east()},
         Vertex{k_points[k_north_east_index], tileset_tile_tx.north_east()}};
}

/* static */ void QuadBasedTilesetTile::default_ramp_properties_loader_strategy
    (const WithPropertiesLoader &)
{
    throw RuntimeError{"properties loader strategy not set"};
}

QuadBasedTilesetTile::QuadBasedTilesetTile(RampPropertiesLoaderStrategy strat):
    m_properties_loader_strategy(strat) {}

const TileCornerElevations & QuadBasedTilesetTile::corner_elevations() const
    { return m_corner_elevations; }

void QuadBasedTilesetTile::make
    (ProducableTileCallbacks & callbacks) const
{
    callbacks.
        add_entity().
        add(SharedPtr<const Texture>{m_texture_ptr}).
        add(SharedPtr<const RenderModel>{m_render_model}).
        finish();
    callbacks.add_collidable(TriangleSegment
        {m_vertices[m_elements[0]].position,
         m_vertices[m_elements[1]].position,
         m_vertices[m_elements[2]].position});
    callbacks.add_collidable(TriangleSegment
        {m_vertices[m_elements[3]].position,
         m_vertices[m_elements[4]].position,
         m_vertices[m_elements[5]].position});

    (void)TileDecorationCreation::create_tile_decoration_with(callbacks);
}

void QuadBasedTilesetTile::make
    (const NeighborCornerElevations &,
     ProducableTileCallbacks & callbacks) const
{ make(callbacks); }

void QuadBasedTilesetTile::setup
    (const TilesetTileTexture & tileset_tile_texture,
     const TileCornerElevations & elevations,
     PlatformAssetsStrategy & platform)
{
    auto model = platform.make_render_model();
    const auto vertices = elevate(make_vertices(tileset_tile_texture), elevations);

    model->load
        (vertices.begin(), vertices.end(), m_elements.cbegin(), m_elements.cend());
    m_corner_elevations = elevations;
    m_texture_ptr = tileset_tile_texture.texture();
    m_render_model = model;
    m_vertices = vertices;
}

void QuadBasedTilesetTile::load
    (const MapTilesetTile & tileset_tile,
     const TilesetTileTexture & tile_texture,
     PlatformAssetsStrategy & platform)
{
    with_loader([&, this](RampPropertiesLoaderBase & loader) {
        loader.load(tileset_tile);
        set_orientation(loader.elements_orientation());
        setup(tile_texture, loader.corner_elevations(), platform);
    });
}

void QuadBasedTilesetTile::set_orientation(Orientation orientation) {
    using O = Orientation;
    switch (orientation) {
    case O::any_elements: case O::nw_to_se_elements:
        m_elements = k_nw_to_se_elements;
        break;
    case O::sw_to_ne_elements:
        m_elements = k_sw_to_ne_elements;
        break;
    }
}

template <typename Func>
/* private */ void QuadBasedTilesetTile::with_loader(Func && f) const {
    class Impl final : public WithPropertiesLoader {
    public:
        explicit Impl(Func && f_): m_f(std::move(f_)) {}

        void operator() (RampPropertiesLoaderBase & base) const final
            { m_f(base); }

    private:
        Func m_f;
    };

    Impl impl{std::move(f)};
    m_properties_loader_strategy(impl);
}

// ----------------------------------------------------------------------------

/* static */ Optional<TileCornerElevations>
    RampPropertiesLoaderBase::read_elevation_of
    (const MapTilesetTile & tileset_tile)
{
    if (auto elv = tileset_tile.get_numeric_property<Real>("elevation")) {
        return TileCornerElevations{*elv, *elv, *elv, *elv};
    }
    return TileCornerElevations{0, 0, 0, 0};
}

/* static */ Optional<CardinalDirection>
    RampPropertiesLoaderBase::read_direction_of
    (const MapTilesetTile & tileset_tile)
{
    return cardinal_direction_from
        (tileset_tile.get_string_property("direction"));
}

void RampPropertiesLoaderBase::load(const MapTilesetTile & tile) {
    auto elevations = read_elevation_of(tile);
    auto direction  = read_direction_of(tile);
    if (elevations) {
        m_elevations = *elevations;
    } else {
        m_elevations = TileCornerElevations{};
    }
    if (direction) {
        m_elevations = m_elevations.add(elevation_offsets_for(*direction));
        m_orientation = orientation_for(*direction);
    }
}

const TileCornerElevations & RampPropertiesLoaderBase::corner_elevations() const
    { return m_elevations; }

namespace {

Optional<CardinalDirection> cardinal_direction_from(const char * nullable_str) {
    static const auto k_strings_as_directions = [] {
        using Cd = CardinalDirection;
        std::map<std::string, CardinalDirection> rv {
            { "n"         , Cd::north      },
            { "s"         , Cd::south      },
            { "e"         , Cd::east       },
            { "w"         , Cd::west       },
            { "ne"        , Cd::north_east },
            { "nw"        , Cd::north_west },
            { "se"        , Cd::south_east },
            { "sw"        , Cd::south_west },
            { "north"     , Cd::north      },
            { "south"     , Cd::south      },
            { "east"      , Cd::east       },
            { "west"      , Cd::west       },
            { "north-east", Cd::north_east },
            { "north-west", Cd::north_west },
            { "south-east", Cd::south_east },
            { "south-west", Cd::south_west },
        };
        return rv;
    } ();
    if (nullable_str) {
        auto itr = k_strings_as_directions.find(nullable_str);
        if (itr != k_strings_as_directions.end()) {
            return itr->second;
        }
    }
    return {};
}

} // end of <anonymous> namespace
