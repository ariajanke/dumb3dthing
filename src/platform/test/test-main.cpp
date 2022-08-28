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

#include "test-functions.hpp"

#include <array>
#include <algorithm>

int main() {
    using BoolFunc = bool(*)();

    // I *could* just dump all my test functions here I guess...
    const std::array k_test_functions = {
        run_triangle_segment_tests,
        run_map_loader_tests,
        run_triangle_links_tests,
        run_systems_tests
    };
    bool all_ok = std::all_of(k_test_functions.begin(), k_test_functions.end(),
                [](BoolFunc f){ return f(); });
    return all_ok ? 0 : ~0;
}
