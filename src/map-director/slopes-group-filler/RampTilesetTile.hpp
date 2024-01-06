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

#include "SlopesTilesetTile.hpp"
#include "FlatTilesetTile.hpp"

class RampPropertiesLoaderBase {
public:
    enum class Orientation { nw_to_se_elements, sw_to_ne_elements, any_elements };

    virtual ~RampPropertiesLoaderBase() {}

    void load(const MapTilesetTile & tile);

    Orientation elements_orientation() const { return m_orientation; }

    const TileCornerElevations & corner_elevations() const;

protected:
    virtual TileCornerElevations elevation_offsets_for
        (CardinalDirection direction) const = 0;

    virtual Orientation orientation_for(CardinalDirection) const = 0;

private:
    Orientation m_orientation;
    TileCornerElevations m_elevations;
};

// ----------------------------------------------------------------------------

class RampPropertiesLoader final : public RampPropertiesLoaderBase {
private:
    TileCornerElevations elevation_offsets_for
        (CardinalDirection direction) const final
    {
        using Cd = CardinalDirection;
        switch (direction) {
        case Cd::north: return TileCornerElevations{1, 1, 0, 0};
        case Cd::east : return TileCornerElevations{1, 0, 0, 1};
        case Cd::south: return TileCornerElevations{0, 0, 1, 1};
        case Cd::west : return TileCornerElevations{0, 1, 1, 0};
        default:
            throw InvalidArgument{"direction bad"};
        }
    }

    Orientation orientation_for(CardinalDirection) const final
        { return Orientation::any_elements; }
};

// ----------------------------------------------------------------------------

class RampTileseTile final : public SlopesTilesetTile {
public:
    static Optional<CardinalDirection> read_direction_of
        (const MapTilesetTile &);

    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const NeighborCornerElevations &,
         ProducableTileCallbacks & callbacks) const;

private:
    QuadBasedTilesetTile m_quad_tileset_tile;
};
