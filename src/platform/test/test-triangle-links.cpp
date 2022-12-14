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

#include "../../TriangleLink.hpp"
#include "../../PointAndPlaneDriver.hpp"

#include <ariajanke/cul/TestSuite.hpp>

bool run_triangle_links_tests() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    using namespace cul::exceptions_abbr;
    using Triangle = TriangleSegment;
    using Side = TriangleSegment::Side;
    using Vec2 = Vector2;
    using std::get;
    static auto make_tri = [](Vec2 a, Vec2 b, Vec2 c) {
        auto to_v3 = [](Vec2 r) { return Vector{r.x, r.y, 0}; };
        return make_shared<TriangleLink>(to_v3(a), to_v3(b), to_v3(c));
    };
    TestSuite suite;
    suite.start_series("TriangleLinks");
    mark(suite).test([] {
        auto triangle_a = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{ 1, 1});
        auto triangle_b = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{-1, 0});
        TriangleLink links (triangle_a->segment());
        links.attempt_attachment_to(triangle_b);
        return test(    links.transfers_to(Side::k_side_ab).target == triangle_b
                    && !links.transfers_to(Side::k_side_bc).target
                    && !links.transfers_to(Side::k_side_ca).target);
    });

    mark(suite).test([] {
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

        links_b->attempt_attachment_to(links_a);
        auto trans = links_b->transfers_to(Side::k_side_ab);
        return test(    trans.target && !trans.inverts_normal
                    && !links_b->transfers_to(Side::k_side_bc).target
                    && !links_b->transfers_to(Side::k_side_ca).target);
    });
    mark(suite).test([] {
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

        links_a->attempt_attachment_to(links_b);
        auto trans = links_a->transfers_to(Side::k_side_ca);
        return test(    trans.target && !trans.inverts_normal
                    && !links_a->transfers_to(Side::k_side_bc).target
                    && !links_a->transfers_to(Side::k_side_ab).target);
    });
    mark(suite).test([] {
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

        links_a->attempt_attachment_to(links_b);
        auto a_trans = links_a->transfers_to(Side::k_side_ca);

        links_b->attempt_attachment_to(links_a);
        auto b_trans = links_b->transfers_to(Side::k_side_ab);

        return test(    a_trans.target && !a_trans.inverts_normal
                    && !links_a->transfers_to(Side::k_side_bc).target
                    && !links_a->transfers_to(Side::k_side_ab).target
                    &&  b_trans.target && !b_trans.inverts_normal
                    && !links_b->transfers_to(Side::k_side_bc).target
                    && !links_b->transfers_to(Side::k_side_ca).target);
    });
    // something else?
    // false positive
    mark(suite).test([] {
        Vector2 displacement{0.018206371897582618,0.018211294926158639};
        Vector2 location{0.35605308997654295, 0.35604975301640995};
        Triangle triangle{Vector{2.5, 0, 0.5}, Vector{3.5, 0, -0.5}, Vector{3.5, 0, 0.5}};
        auto new_loc_ = location + displacement;
        // deeper inside... ruled as parallel to side of the triangle
        auto inside_maybe = triangle.check_for_side_crossing(location, new_loc_).side;
        if (inside_maybe != Side::k_inside) return test(true);
        // if "side_crossing" returns k_inside, then if must contain "new_loc_"
        return test(triangle.contains_point(new_loc_));
    });
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
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        // these triangle's normals are anti-parallel
        Triangle lhs
            {Vector{0, 0, -0.5}, Vector{1, 1, -1.5}, Vector{1, 0, -0.5}};
        Triangle rhs
            {Vector{0, 1, 0.5}, Vector{0, 0, -0.5}, Vector{1, 0, -0.5}};

        auto links_lhs = make_shared<TriangleLink>(lhs);
        auto links_rhs = make_shared<TriangleLink>(rhs);

        unit.start(mark(suite), [&] {
            links_lhs->attempt_attachment_to(links_rhs);
            return test(links_lhs->has_side_attached(Side::k_side_ca));
        });
        unit.start(mark(suite), [&] {
            links_lhs->attempt_attachment_to(links_rhs);
            auto trans = links_lhs->transfers_to(Side::k_side_ca);
            return test(!trans.inverts_normal);
        });
    });
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        // normals that are orthogonal
        Triangle lhs{
            Vector{0, 0, 0}, Vector{0, 0, 1}, Vector{1, 0, 0}};
        Triangle rhs{
            Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}};

        auto links_lhs = make_shared<TriangleLink>(lhs);
        auto links_rhs = make_shared<TriangleLink>(rhs);
        links_lhs->attempt_attachment_to(links_rhs);

        unit.start(mark(suite), [&] {
            auto ang = angle_between(lhs.normal(), rhs.normal());
            return test(are_very_close(ang, k_pi*0.5));
        });

        unit.start(mark(suite), [&] {
            auto trans = links_lhs->transfers_to(Side::k_side_ca);
            return test(!trans.inverts_normal);
        });
    });
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        // normal: <x: 0, y: -1, z: 0> bc
        Triangle floor
            {Vector{10.5, 0, 14.5}, Vector{11.5, 0, 13.5}, Vector{11.5, 0, 14.5}};
        // normal: <x: -1, y: 0, z: 0> ab
        Triangle wall
            {Vector{11.5, 0, 13.5}, Vector{11.5, 0, 14.5}, Vector{11.5, 1, 13.5}};
        auto links_floor = make_shared<TriangleLink>(floor);
        auto links_wall = make_shared<TriangleLink>(wall);

        links_floor->attempt_attachment_to(links_wall);
        links_wall->attempt_attachment_to(links_floor);
        unit.start(mark(suite), [&] {
            // if you flip one way, you must flip the other
            auto floor_trans = links_floor->transfers_to(Side::k_side_bc);
            auto wall_trans = links_wall->transfers_to(Side::k_side_ab);
            return test(floor_trans.inverts_normal == wall_trans.inverts_normal);
        });
    });
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        // parallel normals, should not invert
        // normal x: 0, y: 0, z: -1 ab
        Triangle lhs{Vector{1.5, 2, 6.5}, Vector{2.5, 2, 6.5}, Vector{1.5, 3, 6.5}};
        // normal x: 0, y: 0, z: -1 bc
        Triangle rhs{Vector{2.5, 1, 6.5}, Vector{1.5, 2, 6.5}, Vector{2.5, 2, 6.5}};
        auto links_lhs = make_shared<TriangleLink>(lhs);
        auto links_rhs = make_shared<TriangleLink>(rhs);
        links_lhs->attempt_attachment_to(links_rhs);
        links_rhs->attempt_attachment_to(links_lhs);

        assert(!are_very_close(lhs.normal(), rhs.normal()));
        unit.start(mark(suite), [&] {
            auto trans = links_lhs->transfers_to(Side::k_side_ab);
            return test(trans.target && trans.inverts_normal);
        });
        unit.start(mark(suite), [&] {
            auto trans = links_rhs->transfers_to(Side::k_side_bc);
            return test(trans.target && trans.inverts_normal);
        });
    });
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
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

        unit.start(mark(suite), [&] {
            links_lhs->attempt_attachment_to(links_rhs);
            auto trans = links_lhs->transfers_to(Side::k_side_ab);
            return test(!!trans.target);
        });

        unit.start(mark(suite), [&] {
            links_lhs->attempt_attachment_to(links_rhs);
            auto trans = links_lhs->transfers_to(Side::k_side_ab);
            return test(trans.inverts_normal);
        });

        unit.start(mark(suite), [&] {
            links_rhs->attempt_attachment_to(links_lhs);
            auto trans = links_rhs->transfers_to(Side::k_side_bc);
            return test(trans.target && trans.inverts_normal);
        });
    });
    suite.start_series("TriangleLink::angle_of_rotation_for_left");
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        VectorRotater rotator{k_up};
        auto angle = TriangleLink::angle_of_rotation_for_left_to_right
            (Vector{}, k_east, k_north, rotator);
        unit.start(mark(suite), [&] {
            auto res = rotator(k_east, angle);
            return test(are_very_close(res, k_north));
        });
        unit.start(mark(suite), [&] {
            return test(are_very_close(angle, -k_pi*0.5));
        });
    });
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        VectorRotater rotator{k_east};
        Vector left {0, 0 , 0.5};
        Vector right{0, 2, -0.5};
        Vector pivot{0, 1, -0.5};
        auto angle = TriangleLink::angle_of_rotation_for_left_to_right
            (pivot, left, right, rotator);
        unit.start(mark(suite), [&] {
            auto res = rotator(left - pivot, angle);
            return test(are_very_close(angle_between(res, right - pivot), 0));
        });
        unit.start(mark(suite), [&] {
            return test(angle < -k_pi*0.5 && angle > -k_pi);
        });
    });

#   undef mark
    return suite.has_successes_only();
}
