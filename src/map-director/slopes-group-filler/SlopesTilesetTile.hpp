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

#include "../../Definitions.hpp"
#include "../../RenderModel.hpp"
#include "../ProducableGrid.hpp"

class MapTilesetTile;
class TileCornerElevations;
class TilesetTileTexture;
class MapTileset;

// ----------------------------------------------------------------------------

enum class CardinalDirection {
    north,
    south,
    east,
    west,
    north_west,
    south_west,
    south_east,
    north_east
};

// ----------------------------------------------------------------------------

class NeighborCornerElevations final {
public:
    class NeighborElevations {
    public:
        static NeighborElevations & null_instance();

        virtual ~NeighborElevations() {}

        virtual TileCornerElevations elevations_from
            (const Vector2I &, CardinalDirection) const = 0;
    };

    Optional<Real> north_east() const;

    Optional<Real> north_west() const;

    Optional<Real> south_east() const;

    Optional<Real> south_west() const;

    void set_neighbors
        (const Vector2I & location_on_map, const NeighborElevations & elvs)
    {
        m_location = location_on_map;
        m_neighbors = &elvs;
    }

private:
    TileCornerElevations elevations_from(CardinalDirection cd) const;

    Vector2I m_location;
    const NeighborElevations * m_neighbors = &NeighborElevations::null_instance();
};

// ----------------------------------------------------------------------------

class TileCornerElevations final {
public:
    TileCornerElevations() {}

    TileCornerElevations
        (Optional<Real> ne_,
         Optional<Real> nw_,
         Optional<Real> sw_,
         Optional<Real> se_);

    bool operator == (const TileCornerElevations & rhs) const noexcept;

    bool operator != (const TileCornerElevations & rhs) const noexcept;

    Optional<Real> north_east() const { return as_optional(m_ne); }

    Optional<Real> north_west() const { return as_optional(m_nw); }

    Optional<Real> south_east() const { return as_optional(m_se); }

    Optional<Real> south_west() const { return as_optional(m_sw); }

    TileCornerElevations add(const TileCornerElevations &) const;

    TileCornerElevations value_or(const NeighborCornerElevations &) const;

private:
    static Optional<Real> add
        (const Optional<Real> & lhs, const Optional<Real> & rhs);

    static Optional<Real> value_or
        (const Optional<Real> & lhs, const Optional<Real> & rhs);

    template <Optional<Real>(NeighborCornerElevations::*kt_corner_f)() const>
    static Optional<Real> value_or
        (const Optional<Real> & lhs, const NeighborCornerElevations & rhs);

    static Optional<Real> as_optional(Real);

    bool are_same(const TileCornerElevations &) const noexcept;

    Real m_nw = k_inf, m_ne = k_inf, m_sw = k_inf, m_se = k_inf;
};

// ----------------------------------------------------------------------------

class SlopesAssetsRetrieval : public PlatformAssetsStrategy {
public:
    struct TileDecoration {
        using ComponentSet =
            TupleBuilder<SharedPtr<const Texture>, SharedPtr<const RenderModel>>;
        Vector2 spawn_position_on_tile;
        ComponentSet components;
    };

    virtual Optional<TileDecoration> tile_decoration
        (Real probability = 0.3) const = 0;
};

// ----------------------------------------------------------------------------

class SlopesTilesetTile {
public:
    virtual void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) = 0;

    virtual const TileCornerElevations & corner_elevations() const = 0;

    // can add the optimization that producables are created from a vector
    virtual void make
        (const NeighborCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const = 0;
};

// ----------------------------------------------------------------------------

class TilesetTileTexture final {
public:
    TilesetTileTexture() {}

    TilesetTileTexture
        (const SharedPtr<const Texture> & texture_ptr,
         const Vector2 & north_west,
         const Size2 & tile_size_in_portions);

    void load_texture(const MapTileset &, PlatformAssetsStrategy &);

    void set_texture_bounds(const Vector2I & location_on_tileset);

    Vector2 north_east() const;

    Vector2 north_west() const;

    Vector2 south_east() const;

    Vector2 south_west() const;

    const SharedPtr<const Texture> & texture() const;

    Vertex interpolate(Vertex) const;

private:
    SharedPtr<const Texture> m_texture;
    Vector2 m_north_west;
    Size2 m_tile_size_in_portions;
};
