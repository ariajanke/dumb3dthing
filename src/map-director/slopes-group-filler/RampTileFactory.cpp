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

#include "RampTileFactory.hpp"

#include "../TileSetPropertiesGrid.hpp"

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

void SingleModelSlopedTileFactory::operator ()
    (EntityAndTrianglesAdder & adder, const SlopeGroupNeighborhood & nhood,
     Platform & platform) const
{
    auto r = nhood.tile_location_on_field();

    TileFactory::add_triangles_based_on_model_details
        (r, translation(), model_tile_elevations(), adder);
    adder.add_entity(make_entity(platform, r));
}

/* protected */ void SingleModelSlopedTileFactory::setup_
    (const Vector2I & loc_in_ts, const TileProperties & properties,
     Platform & platform)
{
    TranslatableTileFactory::setup_(loc_in_ts, properties, platform);
    m_render_model = make_render_model_with_common_texture_positions(
        platform, model_tile_elevations(), loc_in_ts);
}

// ----------------------------------------------------------------------------

/* private */ void RampTileFactory::setup_
    (const Vector2I & loc_in_ts, const TileProperties & properties,
     Platform & platform)
{
    if (const auto * val = properties.find_value("direction")) {
        set_direction(val->c_str());
    }
    SingleModelSlopedTileFactory::setup_(loc_in_ts, properties, platform);
}

// --------------------------- <anonymous> namespace --------------------------

/* protected */ void CornerRampTileFactory::set_direction(const char * dir) {
    using Cd = CardinalDirection;
    int n = [dir] {
        switch (cardinal_direction_from(dir)) {
        case Cd::nw: return 0;
        case Cd::sw: return 1;
        case Cd::se: return 2;
        case Cd::ne: return 3;
        default: throw BadBranchException{__LINE__, __FILE__};
        }
    } ();
    m_slopes = half_pi_rotations(non_rotated_slopes(), n);
}

// --------------------------- <anonymous> namespace --------------------------

/* private */ void TwoRampTileFactory::set_direction(const char * dir) {
    static const Slopes k_non_rotated_slopes{1, 1, 0, 0};
    using Cd = CardinalDirection;
    int n = [dir] {
        switch (cardinal_direction_from(dir)) {
        case Cd::n: return 0;
        case Cd::w: return 1;
        case Cd::s: return 2;
        case Cd::e: return 3;
        default: throw BadBranchException{__LINE__, __FILE__};
        }
    } ();
    m_slopes = half_pi_rotations(k_non_rotated_slopes, n);
}
