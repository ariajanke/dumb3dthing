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

#include "../../Definitions.hpp"
#include "../../RenderModel.hpp"
#include "../ProducableGrid.hpp"

class MapTilesetTile;

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

class TileCornerElevations;

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


class TileCornerElevations final {
public:
    TileCornerElevations() {}

    TileCornerElevations
        (Real ne_, Real nw_, Real sw_, Real se_):
        m_nw(nw_), m_ne(ne_), m_sw(sw_), m_se(se_) {}

    TileCornerElevations
        (Optional<Real> ne_,
         Optional<Real> nw_,
         Optional<Real> sw_,
         Optional<Real> se_):
        m_nw(nw_.value_or(k_inf)),
        m_ne(ne_.value_or(k_inf)),
        m_sw(sw_.value_or(k_inf)),
        m_se(se_.value_or(k_inf)) {}

    bool operator == (const TileCornerElevations & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const TileCornerElevations & rhs) const noexcept
        { return !are_same(rhs); }

    Optional<Real> north_east() const { return as_optional(m_ne); }

    Optional<Real> north_west() const { return as_optional(m_nw); }

    Optional<Real> south_east() const { return as_optional(m_se); }

    Optional<Real> south_west() const { return as_optional(m_sw); }

    TileCornerElevations add(const TileCornerElevations & rhs) const {
        return TileCornerElevations
            {add(north_east(), rhs.north_east()),
             add(north_west(), rhs.north_west()),
             add(south_west(), rhs.south_west()),
             add(south_east(), rhs.south_east())};
    }

    TileCornerElevations value_or(const NeighborCornerElevations & rhs) const {
        using Nce = NeighborCornerElevations;
        return TileCornerElevations
            {value_or<&Nce::north_east>(north_east(), rhs),
             value_or<&Nce::north_west>(north_west(), rhs),
             value_or<&Nce::south_west>(south_west(), rhs),
             value_or<&Nce::south_east>(south_east(), rhs)};
    }

private:
    static Optional<Real> add
        (const Optional<Real> & lhs, const Optional<Real> & rhs)
    {
        if (!lhs && !rhs)
            { return {}; }
        return lhs.value_or(0) + rhs.value_or(0);
    }

    static Optional<Real> value_or
        (const Optional<Real> & lhs, const Optional<Real> & rhs)
    { return lhs ? lhs : rhs; }

    template <Optional<Real>(NeighborCornerElevations::*kt_corner_f)() const>
    static Optional<Real> value_or
        (const Optional<Real> & lhs, const NeighborCornerElevations & rhs)
    {
        if (lhs) {
            return lhs;
        }
        return (rhs.*kt_corner_f)();
    }


    static Optional<Real> as_optional(Real r) {
        if (std::equal_to<Real>{}(r, k_inf))
            { return {}; }
        return r;
    }

    bool are_same(const TileCornerElevations & rhs) const noexcept {
        using Fe = std::equal_to<Real>;
        return    Fe{}(m_nw, rhs.m_nw) && Fe{}(m_ne, rhs.m_ne)
               && Fe{}(m_sw, rhs.m_sw) && Fe{}(m_se, rhs.m_se);
    }

    Real m_nw = k_inf, m_ne = k_inf, m_sw = k_inf, m_se = k_inf;
};

/* static */ inline NeighborCornerElevations::NeighborElevations &
    NeighborCornerElevations::NeighborElevations::null_instance()
{
    class Impl final : public NeighborElevations {
        TileCornerElevations elevations_from
            (const Vector2I &, CardinalDirection) const final
        { throw RuntimeError{""}; }
    };
    static Impl impl;
    return impl;
}

class TilesetTileTexture;

class SlopesTilesetTile {
public:
    virtual void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) = 0;

    virtual TileCornerElevations corner_elevations() const = 0;

    // can add the optimization that producables are created from a vector
    virtual void make
        (const NeighborCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const = 0;
};

class MapTileset;

class TilesetTileTexture final {
public:
    TilesetTileTexture() {}

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

class ProducableSlopesTile final : public ProducableTile {
public:
    ProducableSlopesTile() {}

    explicit ProducableSlopesTile
        (const SharedPtr<const SlopesTilesetTile> & tileset_tile_ptr):
        m_tileset_tile_ptr(tileset_tile_ptr) {}

    void set_neighboring_elevations(const NeighborCornerElevations & elvs) {
        m_elevations = elvs;
    }

    void operator () (ProducableTileCallbacks & callbacks) const final {
        if (m_tileset_tile_ptr)
            m_tileset_tile_ptr->make(m_elevations, callbacks);
    }

private:
    const SlopesTilesetTile & verify_tile_ptr() const {
        if (m_tileset_tile_ptr)
            { return *m_tileset_tile_ptr; }
        throw RuntimeError
            {"Accessor not available without setting tileset tile pointer"};
    }

    SharedPtr<const SlopesTilesetTile> m_tileset_tile_ptr;
    NeighborCornerElevations m_elevations;
};
