/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "../../../src/targeting-state/TargetingState.hpp"
#include "../../../tests/test-helpers.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<TargetSeekerCone>("TargetSeekerCone#contains")([] {
    TargetSeekerCone cone{k_up, Vector{}, k_pi / 4.};
    mark_it("does not contain, outside by distance alone", [&cone] {
        Vector pt = -k_up*0.01;
        return test_that(!cone.contains(pt));
    }).
    mark_it("does not contain, outside by angle alone", [&cone] {
        Vector pt = (k_east + k_north)*0.9;
        return test_that(!cone.contains(pt));
    }).
    mark_it("does contain", [&cone] {
        Vector pt = k_up*0.01 + (k_east + k_north)*(1. / std::sqrt(2))*0.98;
        return test_that(cone.contains(pt));
    });
});

describe<TargetingState>("TargetingState::interval_of")([] {
    mark_it("tip to base runs along x-axis only", [] {
        TargetSeekerCone cone{k_east, Vector{}, k_pi / 4.};
        auto res = TargetingState::interval_of(cone);
        return test_that(are_very_close(res.low , 0.) &&
                         are_very_close(res.high, 1.));
    }).
    mark_it("tip to base runs along y-axis only", [] {
        TargetSeekerCone cone{k_up, Vector{}, k_pi / 4.};
        auto res = TargetingState::interval_of(cone);
        return test_that(are_very_close(res.low , -1.) &&
                         are_very_close(res.high,  1.));
    }).
    mark_it("tip to base runs along two axises", [] {
        TargetSeekerCone cone
            {Vector{}, (k_up + k_east)*(1. / std::sqrt(2.)), k_pi / 4.};
        auto res = TargetingState::interval_of(cone);
        return test_that(are_very_close(res.low , 0.) &&
                         are_very_close(res.high, 2. / std::sqrt(2.)));
    }).
    mark_it("tip to base runs along all three axises", [] {
        // I think this is actually correct
        TargetSeekerCone cone
            {Vector{}, (k_up + k_north + k_east)*(1. / std::sqrt(3.)), k_pi / 4.};
        auto res = TargetingState::interval_of(cone);
        return test_that(are_very_close(res.low , 0.) &&
                         are_very_close(res.high, 1. / std::sqrt(3.)));
    });
});

return [] {};

}();
