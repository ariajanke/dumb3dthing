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

#include "../../src/point-and-plane/FrameTimeLinkContainer.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

#define mark_it mark_source_position(__LINE__, __FILE__).it

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
describe<FrameTimeLinkContainer>("FrameTimeLinkContainer")([] {
    FrameTimeLinkContainer ftlc;
    auto a = make_shared<TriangleLink>();
    auto b = make_shared<TriangleLink>();
    auto c = make_shared<TriangleLink>();
    auto make_view = [&ftlc]
        { return ftlc.view_for(-Vector{1, 1, 1}*1000, Vector{1, 1, 1}*1000); };
    for (auto p : { a, c, b })
        ftlc.defer_addition_of(p);
    // this is why nested contexts are nice
    mark_it("defers addition of a; update adds the object", [&] {
        ftlc.update();
        auto view = make_view();
        auto a_res = std::find(view.begin(), view.end(), a);
        return test_that(a_res != view.end());
    });
    mark_it("defers addition of c; update adds this object too", [&] {
        ftlc.update();
        auto view = make_view();
        auto c_res = std::find(view.begin(), view.end(), c);
        return test_that(c_res != view.end());
    });
    mark_it("defer removal of, removes a previously added object at update", [&] {
        ftlc.defer_removal_of(b);
        ftlc.update();
        auto view = make_view();
        auto b_res = std::find(view.begin(), view.end(), b);
        return test_that(b_res == view.end());
    });
});

class Dummy final {};
return Dummy{};
} ();
