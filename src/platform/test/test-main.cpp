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

#include <vector>
#include <memory>
#include <atomic>

int main() {
    // I *could* just dump all my test functions here I guess...
    const std::array k_test_functions = {
        run_spm_tests,
        run_triangle_segment_tests,
        run_map_loader_tests,
        run_triangle_links_tests,
        run_systems_tests,
        run_wall_tile_factory_tests,
    };
    std::array<bool, k_test_functions.size()> results;
    for (std::size_t i = 0; i != k_test_functions.size(); ++i)
        results[i] = k_test_functions[i]();
    bool all_ok = std::all_of(results.begin(), results.end(),
                [](bool b){ return b; });
#   if 0
    {
    std::vector<int> shit;
    shit.resize(100, 120);
    std::vector<std::shared_ptr<int>> shit_ptrs;
    shit_ptrs.reserve(100);

    class SharedOwner final {
    public:
        SharedOwner() {}
#       if 0
        explicit SharedOwner(std::vector<std::shared_ptr<int>> && vec):
            shit_ptrs(std::move(vec))
        { counter = shit_ptrs.size(); }
#       endif
        ~SharedOwner() {
            int i = 0;
            ++i;
        }
#       if 0
        std::vector<std::shared_ptr<int>> shit_ptrs;
#       endif
        std::atomic_int counter = 0;
    };

    class SharedDeleter final {
    public:
        SharedDeleter(SharedOwner * ptr): owner(ptr) {}

        void operator () (int *) const {
            --owner->counter;
            if (!owner->counter) {
                delete owner;
            }
        }

        mutable SharedOwner * owner;
    };

    auto vec_owner = new SharedOwner;
    for (auto & shit_ : shit) {
        shit_ptrs.push_back(std::shared_ptr<int>{&shit_, SharedDeleter{vec_owner}});
    }
    vec_owner->counter = shit_ptrs.size();
    }
#   endif
    return all_ok ? 0 : ~0;
}
