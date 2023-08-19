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

#include "../Definitions.hpp"

namespace twist_loop_filler_names {

constexpr const auto k_ns_twist_loop = "ns-twist-loop";
constexpr const auto k_ew_twist_loop = "ew-twist-loop";

constexpr const auto k_name_list = { k_ns_twist_loop, k_ew_twist_loop };

} // end of twist_loop_filler_names namespace


class TileSetXmlGrid;
class ProducableGroupFiller;
class Platform;

class TwistLoopGroupFiller_ final {
public:
    static SharedPtr<ProducableGroupFiller> make
        (const TileSetXmlGrid & xml_grid, Platform &);

private:
    TwistLoopGroupFiller_() {}
};
