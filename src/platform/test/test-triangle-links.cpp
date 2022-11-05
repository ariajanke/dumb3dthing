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
        auto links_a = make_shared<TriangleLink>(triangle_a);
        auto links_b = make_shared<TriangleLink>(triangle_b);

        links_b->attempt_attachment_to(links_a);
        auto trans = links_b->transfers_to(Side::k_side_ab);
        return test(    trans.target && trans.inverts
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

        auto links_a = make_shared<TriangleLink>(triangle_a);
        auto links_b = make_shared<TriangleLink>(triangle_b);

        links_a->attempt_attachment_to(links_b);
        auto trans = links_a->transfers_to(Side::k_side_ca);
        return test(    trans.target && trans.inverts
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

        auto links_a = make_shared<TriangleLink>(triangle_a);
        auto links_b = make_shared<TriangleLink>(triangle_b);

        links_a->attempt_attachment_to(links_b);
        auto a_trans = links_a->transfers_to(Side::k_side_ca);

        links_b->attempt_attachment_to(links_a);
        auto b_trans = links_b->transfers_to(Side::k_side_ab);

        return test(    a_trans.target && a_trans.inverts
                    && !links_a->transfers_to(Side::k_side_bc).target
                    && !links_a->transfers_to(Side::k_side_ab).target
                    &&  b_trans.target && b_trans.inverts
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
    mark(suite).test([] {
        Triangle lhs{
            Vector{0, 0, -0.5}, Vector{1, 1, -1.5}, Vector{1, 0, -0.5}};
        Triangle rhs{
            Vector{0, 1, 0.5}, Vector{0, 0, -0.5}, Vector{1, 0, -0.5}};

        auto links_lhs = make_shared<TriangleLink>(lhs);
        auto links_rhs = make_shared<TriangleLink>(rhs);

        links_lhs->attempt_attachment_to(links_rhs);
        return test(links_lhs->has_side_attached(Side::k_side_ca));
    });

#   undef mark
    return suite.has_successes_only();
}
