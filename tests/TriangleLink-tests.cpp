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

#include "../src/TriangleLink.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

#define mark_it mark_source_position(__LINE__, __FILE__).it

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
using namespace cul::exceptions_abbr;
using Triangle = TriangleSegment;
using Side = TriangleSegment::Side;
using Vec2 = Vector2;
using std::get;
static auto make_tri = [](Vec2 a, Vec2 b, Vec2 c) {
    auto to_v3 = [](Vec2 r) { return Vector{r.x, r.y, 0}; };
    return make_shared<TriangleLink>(to_v3(a), to_v3(b), to_v3(c));
};
describe<TriangleLink>("TriangleLink::reattach_matching_points")([] {
    mark_it("attaches to only one side", [] {
        auto triangle_a = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{ 1, 1});
        auto triangle_b = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{-1, 0});
        TriangleLink::reattach_matching_points(triangle_a, triangle_b);
        return test_that
            (    triangle_a->transfers_to(Side::k_side_ab).target() == triangle_b
             && !triangle_a->transfers_to(Side::k_side_bc).target()
             && !triangle_a->transfers_to(Side::k_side_ca).target());
    });
});
describe<TriangleLink>("TriangleLink::reattach_matching_points")([] {
    // triangle a's ca side to...
    Triangle triangle_a{
        // pt_a             , pt_b                , pt_c
        Vector{2.5, 0, -3.5}, Vector{2.5, 0, -4.5}, Vector{3.5, 0, -4.5}};
    // triangle b's ab
    Triangle triangle_b{
        // pt_a             , pt_c                , pt_d
        Vector{2.5, 0, -3.5}, Vector{3.5, 0, -4.5}, Vector{3.5, 0, -3.5}};
    // note: both triangles have the same normal vector

    auto links_a = make_shared<TriangleLink>(triangle_a);
    auto links_b = make_shared<TriangleLink>(triangle_b);

    TriangleLink::reattach_matching_points(links_a, links_b);

    mark_it("returns a valid transfer object for attached side", [&] {
        auto trans = links_b->transfers_to(Side::k_side_ab);
        return test_that(trans.target() == links_a);
    });
    mark_it("does not invert normal for this case's triangles", [&] {
        return test_that(!links_b->transfers_to(Side::k_side_ab).inverts_normal());
    });
    mark_it("does not attach to any other side", [&] {
        return test_that(   !links_b->transfers_to(Side::k_side_bc).target()
                         && !links_b->transfers_to(Side::k_side_ca).target());
    });
    mark_it("attachs other link to the first", [&] {
        auto a_trans = links_a->transfers_to(Side::k_side_ca);
        return test_that(!!a_trans.target());
    });
});
describe<TriangleLink>("TriangleLink #check_for_side_crossing")([] {
    mark_it("catches a red-green case", [] {
        Vector2 displacement{0.018206371897582618,0.018211294926158639};
        Vector2 location{0.35605308997654295, 0.35604975301640995};
        Triangle triangle{Vector{2.5, 0, 0.5}, Vector{3.5, 0, -0.5}, Vector{3.5, 0, 0.5}};
        auto new_loc_ = location + displacement;
        // deeper inside... ruled as parallel to side of the triangle
        auto inside_maybe = triangle.check_for_side_crossing(location, new_loc_).side;
        if (inside_maybe != Side::k_inside) return test_that(true);
        // if "side_crossing" returns k_inside, then if must contain "new_loc_"
        return test_that(triangle.contains_point(new_loc_));
    });
});
    // some of these are red-green tests...
    // something else?
    // false positive
#   if 0 // I have no idea on handling/testing for the flip-flop case
         // this current one kinda sucks
    mark(suite).test([] {
        // where I want to capture flip-flop
        auto links_a = make_shared<TriangleLink>(Vector{19.5, 1., -.5}, Vector{19.5, 0, -1.5}, Vector{20.5, 0, -1.5});
        auto links_b = make_shared<TriangleLink>(Vector{19.5, 0, -1.5}, Vector{20.5, 0, -2.5}, Vector{20.5, 0, -1.5});

        auto pdriver = [&links_a, &links_b] {
            // need links too
            links_a->attempt_attachment_to(links_b);
            links_b->attempt_attachment_to(links_a);

            auto pdriver = point_and_plane::Driver::make_driver();
            pdriver->add_triangle(links_a);
            pdriver->add_triangle(links_b);
            pdriver->update();
            return pdriver;
        } ();
        auto test_handler = point_and_plane::EventHandler::make_test_handler();

        // first recorded frame
        PpState state{PpOnSegment{links_a, true, Vector2{1.4142019007112767, 0.842617146393735}, Vector2{0.000982092751647734, -0.0762158869304308}}};
        std::cout << segment_displacement_to_v3(state) << std::endl;
        state = (*pdriver)(state, *test_handler);

        get<PpOnSegment>(state).displacement = Vector2{-0.0768356537697602, -0.02994869527758226};
        std::cout << segment_displacement_to_v3(state) << std::endl;
        state = (*pdriver)(state, *test_handler);

        // third frame
        get<PpOnSegment>(state).displacement = Vector2{0.000982092751647956, -0.07479998774150332};
        std::cout << segment_displacement_to_v3(state) << std::endl;
        state = (*pdriver)(state, *test_handler);

        // I can now say for sure the funkiness happens with displacement,
        // therefore it's not a problem with segment transfers

        // a better test -> does constant velocity produce consistent
        // displacements

        // this will require rework of systems a bit to accomodate a test case...
        return test(true); // test is invalid

        // flip-flop seems sourced in this odd flipping back and forth with
        // displacement (how can I test this?)
    });
#   endif
describe<TriangleLink>("TriangleLink::reattach_matching_points (anti-parallel)")([] {
    // these triangle's normals are anti-parallel
    Triangle lhs
        {Vector{0, 0, -0.5}, Vector{1, 1, -1.5}, Vector{1, 0, -0.5}};
    Triangle rhs
        {Vector{0, 1, 0.5}, Vector{0, 0, -0.5}, Vector{1, 0, -0.5}};

    auto links_lhs = make_shared<TriangleLink>(lhs);
    auto links_rhs = make_shared<TriangleLink>(rhs);

    TriangleLink::reattach_matching_points(links_lhs, links_rhs);
    mark_it("attaches to another segment with anti-parallel normal", [&] {
        return test_that(links_lhs->has_side_attached(Side::k_side_ca));
    });
    // note: there are cases of anti-parallel normals that should *not* invert
    //       a tracker
    mark_it("attaches to anti-parallel normal, without inverting", [&] {
        auto trans = links_lhs->transfers_to(Side::k_side_ca);
        return test_that(!trans.inverts_normal());
    });
});
describe<TriangleLink>("TriangleLink::reattach_matching_points (orthogonal)")([] {
    // normals that are orthogonal
    Triangle lhs{
        Vector{0, 0, 0}, Vector{0, 0, 1}, Vector{1, 0, 0}};
    Triangle rhs{
        Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}};

    auto links_lhs = make_shared<TriangleLink>(lhs);
    auto links_rhs = make_shared<TriangleLink>(rhs);
    TriangleLink::reattach_matching_points(links_lhs, links_rhs);

    mark_it("is linking triangle's with orthogonal normals", [&] {
        auto ang = angle_between(lhs.normal(), rhs.normal());
        return test_that(are_very_close(ang, k_pi*0.5));
    });

    mark_it("does not invert tracker normal for this context", [&] {
        auto trans = links_lhs->transfers_to(Side::k_side_ca);
        return test_that(!trans.inverts_normal());
    });
});
describe<TriangleLink>("TriangleLink::reattach_matching_points")([] {
    // normal: <x: 0, y: -1, z: 0> bc
    Triangle floor
        {Vector{10.5, 0, 14.5}, Vector{11.5, 0, 13.5}, Vector{11.5, 0, 14.5}};
    // normal: <x: -1, y: 0, z: 0> ab
    Triangle wall
        {Vector{11.5, 0, 13.5}, Vector{11.5, 0, 14.5}, Vector{11.5, 1, 13.5}};
    auto links_floor = make_shared<TriangleLink>(floor);
    auto links_wall = make_shared<TriangleLink>(wall);

    TriangleLink::reattach_matching_points(links_wall, links_floor);
    mark_it("has consistent inversion flags both ways", [&] {
        // if you flip one way, you must flip the other
        auto floor_trans = links_floor->transfers_to(Side::k_side_bc);
        auto wall_trans = links_wall->transfers_to(Side::k_side_ab);
        return test_that(floor_trans.inverts_normal() ==
                         wall_trans.inverts_normal());
    });
});
describe<TriangleLink>("TriangleLink::reattach_matching_points (equal normals, inverts tracker)")([] {
    // normal x: 0, y: 0, z: -1 ab
    Triangle lhs{Vector{1.5, 2, 6.5}, Vector{2.5, 2, 6.5}, Vector{1.5, 3, 6.5}};
    // normal x: 0, y: 0, z: -1 bc
    Triangle rhs{Vector{2.5, 1, 6.5}, Vector{1.5, 2, 6.5}, Vector{2.5, 2, 6.5}};
    auto links_lhs = make_shared<TriangleLink>(lhs);
    auto links_rhs = make_shared<TriangleLink>(rhs);
    TriangleLink::reattach_matching_points(links_lhs, links_rhs);

    assert(!are_very_close(lhs.normal(), rhs.normal()));
    // split this test
    mark_it("attaches lhs to rhs as target", [&] {
        return test_that
            (links_lhs->transfers_to(Side::k_side_ab).target() == links_rhs);
    });
    mark_it("attaches lhs inverting the tracker", [&] {
        return test_that
            (links_lhs->transfers_to(Side::k_side_ab).inverts_normal());
    });
    mark_it("attaches rhs to lhs as target", [&] {
        return test_that
            (links_rhs->transfers_to(Side::k_side_bc).target() == links_lhs);
    });
    mark_it("attaches lhs inverting the tracker", [&] {
        return test_that
            (links_rhs->transfers_to(Side::k_side_bc).inverts_normal());
    });
});
describe<TriangleLink>("TriangleLink::reattach_matching_points (arbitrary? normals)")([] {
    // flip values are wrong, I believe they need to invert at least from
    // lhs to rhs
    // regular
    // normal x: 0, y: 0, z: 1 ab
    Triangle lhs{Vector{1.5, 1, -0.5}, Vector{2.5, 1, -0.5}, Vector{1.5, 2, -0.5}};
    // regular
    // normal x: 0, y: -0.70711, z: -0.70711 bc
    Triangle rhs{Vector{1.5, 0, 0.5}, Vector{1.5, 1, -0.5}, Vector{2.5, 1, -0.5}};

    auto links_lhs = make_shared<TriangleLink>(lhs);
    auto links_rhs = make_shared<TriangleLink>(rhs);

    TriangleLink::reattach_matching_points(links_lhs, links_rhs);

    mark_it("attaches lhs to rhs", [&] {
        auto trans = links_lhs->transfers_to(Side::k_side_ab);
        return test_that(trans.target() == links_rhs);
    });

    mark_it("inverts tracker normal from lhs to rhs", [&] {
        auto trans = links_lhs->transfers_to(Side::k_side_ab);
        return test_that(trans.inverts_normal());
    });

    mark_it("attaches rhs to lhs", [&] {
        auto trans = links_rhs->transfers_to(Side::k_side_bc);
        return test_that(trans.target() == links_lhs);
    });

    mark_it("inverts tracker normal from rhs to lhs", [&] {
        auto trans = links_rhs->transfers_to(Side::k_side_bc);
        return test_that(trans.inverts_normal());
    });
});

return 1;

} ();
