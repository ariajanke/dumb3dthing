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

#include "../TileFactory.hpp"

enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

class SlopesGridInterface {
public:
    virtual ~SlopesGridInterface() {}

    virtual Slopes operator () (Vector2I) const = 0;

    static const SlopesGridInterface & null_instance() {
        class Impl final : public SlopesGridInterface {
        public:
            Slopes operator () (Vector2I) const final
                { return Slopes{k_inf, k_inf, k_inf, k_inf}; }
        };
        static Impl impl;
        return impl;
    }
};

// there's a type of group'
// each group type is close to each type of tile factory
// therefore a tile factory knows how to create its own group
// TileFactory: virtual NeighborGroup * create_group() const = 0;
//
// do layers have positioning/information that accompany them that affects the
// production of tiles?
// possibly
// is a tile factory local to a tileset? local to a tileset & layer?

struct TileLocation;
class TileGroup;

/// describes neighbors and an address for a tile
class SlopeGroupNeighborhood final {
public:
    SlopeGroupNeighborhood
        (const SlopesGridInterface &, Vector2I tile_loc_on_map,
         Vector2I maps_spawner_offset);

    Real neighbor_elevation(CardinalDirection) const;

    Vector2I tile_location_on_field() const { return m_loc + m_offset; }

    // bad name: is actually more local
    Vector2I tile_location_on_map() const { return m_loc; }

private:
    Real neighbor_elevation(const Vector2I &, CardinalDirection) const;

    const SlopesGridInterface * m_grid = &SlopesGridInterface::null_instance();
    Vector2I m_loc;
    Vector2I m_offset;
};

class TranslatableTileFactory : public TileFactory {
protected:
    Vector translation() const { return m_translation; }

    Entity make_entity
        (Platform & platform, Vector2I tile_loc,
         const SharedPtr<const RenderModel> & model_ptr) const;

    void setup_
        (const Vector2I & loc_in_ts, const TileProperties &, Platform &) override;
private:
    Vector m_translation;
};

class SlopesBasedTileFactory : public TranslatableTileFactory {
public:
    virtual void operator ()
        (EntityAndTrianglesAdder &, const SlopeGroupNeighborhood &,
         Platform &) const = 0;

    virtual Slopes tile_elevations() const = 0;
};

CardinalDirection cardinal_direction_from(const std::string * str);

CardinalDirection cardinal_direction_from(const char * str);
