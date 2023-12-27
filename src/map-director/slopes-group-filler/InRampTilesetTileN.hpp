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

#include "RampTilesetTileN.hpp"

class InRampPropertiesLoader final : public RampPropertiesLoaderBase {
private:
    TileCornerElevations elevation_offsets_for
        (CardinalDirection direction) const final
    {
        using Cd = CardinalDirection;
        switch (direction) {
        case Cd::ne: return TileCornerElevations{1, 1, 0, 1};
        case Cd::nw: return TileCornerElevations{1, 1, 1, 0};
        case Cd::se: return TileCornerElevations{1, 0, 1, 1};
        case Cd::sw: return TileCornerElevations{0, 1, 1, 1};
        default:
            throw InvalidArgument{"direction bad"};
        }
    }

    Orientation orientation_for(CardinalDirection direction) const final {
        using Cd = CardinalDirection;
        switch (direction) {
        case Cd::ne: case Cd::sw:
            return Orientation::sw_to_ne_elements;
        case Cd::nw: case Cd::se:
            return Orientation::nw_to_se_elements;
        default:
            throw InvalidArgument{"direction bad"};
        }
    }
};

// ----------------------------------------------------------------------------

class InRampTilesetTile final : public SlopesTilesetTile {
public:
    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const TileCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;

private:
    QuadBasedTilesetTile m_quad_tile;
};
