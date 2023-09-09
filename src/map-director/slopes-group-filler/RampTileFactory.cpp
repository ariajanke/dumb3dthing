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

#include "../ProducableGrid.hpp"
#include "../TileSetPropertiesGrid.hpp"

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

void SingleModelSlopedTileFactory::operator ()
    (const SlopeGroupNeighborhood &,
     ProducableTileCallbacks & callbacks) const
{
    TileFactory::add_triangles_based_on_model_details
        (translation(), model_tile_elevations(), callbacks);
    add_modeled_entity_with(callbacks).
        get<ModelTranslation>() += translation();
}

/* protected */ void SingleModelSlopedTileFactory::setup_
    (const TileProperties & properties, Platform & platform,
     const SlopeFillerExtra &,
     const Vector2I & location_on_tileset)
{
    TranslatableTileFactory::setup_(location_on_tileset, properties, platform);
    m_render_model = make_render_model_with_common_texture_positions(
        platform, model_tile_elevations(), location_on_tileset);
}

// ----------------------------------------------------------------------------

/* private */ void RampTileFactory::setup_
    (const TileProperties & properties, Platform & platform,
     const SlopeFillerExtra & slope_extras,
     const Vector2I & location_on_tileset)
{
    properties.for_value("direction", [this](const auto & val) {
        set_direction(val.c_str());
    });
    SingleModelSlopedTileFactory::setup_
        (properties, platform, slope_extras, location_on_tileset);
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
