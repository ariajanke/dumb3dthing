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
#include "../ProducableGrid.hpp"
#if 0
class TilesetXmlGrid;
#endif
class MapTilesetTile;

enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

class TileCornerElevations final {
public:
    class NeighborElevations {
    public:
        virtual ~NeighborElevations() {}

        virtual TileCornerElevations elevations_from(CardinalDirection) const = 0;

        TileCornerElevations elevations() const;
    };

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

private:
    static Optional<Real> add
        (const Optional<Real> & lhs, const Optional<Real> & rhs)
    {
        if (!lhs && !rhs)
            { return {}; }
        return lhs.value_or(0) + rhs.value_or(0);
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

class SlopesTilesetTile {
public:
    static SharedPtr<SlopesTilesetTile> make
        (const MapTilesetTile &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform);

    virtual void load
        (const MapTilesetTile &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform) = 0;

    virtual TileCornerElevations corner_elevations() const = 0;

    // can add the optimization that producables are created from a vector
    virtual void make
        (const TileCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const = 0;
};

class ProducableSlopesTile final : public ProducableTile {
public:
    ProducableSlopesTile() {}

    ProducableSlopesTile
        (const SharedPtr<const SlopesTilesetTile> & tileset_tile_ptr,
         const TileCornerElevations & elevations):
        m_tileset_tile_ptr(tileset_tile_ptr),
        m_elevations(elevations) {}

    TileCornerElevations corner_elevations() const {
        if (m_tileset_tile_ptr)
            return m_tileset_tile_ptr->corner_elevations();
        return TileCornerElevations{};
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
    TileCornerElevations m_elevations;
};
