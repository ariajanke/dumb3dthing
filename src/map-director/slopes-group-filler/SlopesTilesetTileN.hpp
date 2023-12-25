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

class TilesetXmlGrid;

enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

class TileCornerElevations final {
public:
    TileCornerElevations() {}

    TileCornerElevations
        (Real ne_, Real nw_, Real sw_, Real se_):
        m_nw(nw_), m_ne(ne_), m_sw(sw_), m_se(se_) {}

    bool operator == (const TileCornerElevations & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const TileCornerElevations & rhs) const noexcept
        { return !are_same(rhs); }

    Optional<Real> north_east() const { return m_ne; }

    Optional<Real> north_west() const { return m_nw; }

    Optional<Real> south_east() const { return m_se; }

    Optional<Real> south_west() const { return m_sw; }

private:
    bool are_same(const TileCornerElevations & rhs) const noexcept {
        using Fe = std::equal_to<Real>;
        return    Fe{}(m_nw, rhs.m_nw) && Fe{}(m_ne, rhs.m_ne)
               && Fe{}(m_sw, rhs.m_sw) && Fe{}(m_se, rhs.m_se);
    }

    Real m_nw, m_ne, m_sw, m_se;
};
class FlatProducableTile;

class SlopesTilesetTile {
public:
    class Callbacks {
    public:
        virtual void place_flat(const FlatProducableTile &) = 0;
    };

    static SharedPtr<SlopesTilesetTile> make
        (const TilesetXmlGrid &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform);

    virtual ~SlopesTilesetTile() {}

    virtual void load
        (const TilesetXmlGrid &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform) = 0;

    virtual TileCornerElevations corner_elevations() const = 0;

    // can add the optimization that producables are created from a vector
    virtual void make_producable
        (const TileCornerElevations & neighboring_elevations,
         Callbacks & callbacks) const = 0;
};
