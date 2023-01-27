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

/// @file This is a transitionary header. It acts as an interface for the rest
///       of the program. That is sources in this directory, should only access
///       the contents/code of the directory of the same name as this header,
///       through this header.
///

#pragma once

#include "ProducableTileFiller.hpp"

namespace slopes_group_filler_type_names {

static constexpr auto k_in_wall  = "in-wall" ;
static constexpr auto k_out_wall = "out-wall";
static constexpr auto k_wall     = "wall"    ;
static constexpr auto k_in_ramp  = "in-ramp" ;
static constexpr auto k_out_ramp = "out-ramp";
static constexpr auto k_ramp     = "ramp"    ;
static constexpr auto k_flat     = "flat"    ;

static constexpr auto k_ramp_group_type_list = {
    k_in_wall, k_out_wall, k_wall, k_in_ramp, k_out_ramp, k_ramp, k_flat
};

} // end of slopes_group_filler_type_names namespace

class SlopeGroupFiller_ final {
public:
    static SharedPtr<ProducableTileFiller> make
        (const TileSetXmlGrid & xml_grid, Platform &);

private:
    SlopeGroupFiller_() {}
};
