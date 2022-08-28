/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "../../GameDriver.hpp"
#include "../../map-loader.hpp"
#include "../../Systems.hpp"

namespace {

using BoolFunc = bool(*)();

} // end of <anonymous> namespace

int main() {
    // I *could* just dump all my test functions here I guess...
    const std::array k_test_functions = {
        point_and_plane::TriangleLinks::run_tests,
        run_map_loader_tests,
        TriangleSegment::run_tests,
        run_system_tests
    };
    bool all_ok = std::all_of(k_test_functions.begin(), k_test_functions.end(),
                [](BoolFunc f){ return f(); });
    return all_ok ? 0 : ~0;
}
