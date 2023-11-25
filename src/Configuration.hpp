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

#include "Definitions.hpp"

static constexpr const auto k_testmap_filename = "nested-map-demo.tmx";
static constexpr const Vector k_player_start{2, 12.1, -2};
static constexpr const bool
    k_region_axis_container_report_maximum_sort_and_sweep = true;
constexpr const bool k_promised_files_take_at_least_one_frame = false;
