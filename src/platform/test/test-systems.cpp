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
#include "../../Systems.hpp"

#include <ariajanke/cul/TestSuite.hpp>

namespace {

using point_and_plane::EventHandler;
using Triangle = TriangleSegment;

/* static */ UniquePtr<EventHandler> make_test_handler() {
    class TestHandler final : public point_and_plane::EventHandler {
        Variant<Vector2, Vector>
            on_triangle_hit
            (const Triangle &, const Vector &, const Vector2 &, const Vector &) const final
        { return Vector2{}; }

        Variant<Vector, Vector2>
            on_transfer_absent_link
            (const Triangle &, const SideCrossing &, const Vector2 &) const final
        { return Vector{}; }

        Variant<Vector, TransferOnSegment>
            on_transfer
            (const Triangle &, const Triangle::SideCrossing &,
             const Triangle &, const Vector &) const final
        { return make_tuple(Vector2{}, true); }
    };

    return make_unique<TestHandler>();
}

}

bool run_systems_tests() {
    using namespace cul::exceptions_abbr;
    using namespace cul::ts;

    TestSuite suite;
    suite.start_series("Systems Tests");
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    set_context(suite, [](TestSuite & suite, Unit & unit) {
        using std::get;
        using VtoD = VelocitiesToDisplacement;

        auto make_pdriver = []
            (SharedPtr<TriangleLink> links_a, SharedPtr<TriangleLink> links_b)
        {
            TriangleLink::attach_matching_points(links_a, links_b);
            auto pdriver = point_and_plane::Driver::make_driver();
            pdriver->add_triangle(links_a);
            pdriver->add_triangle(links_b);
            pdriver->update();
            return pdriver;
        };
#       if 1
        auto test_handler = make_test_handler();
#       endif
#       if 1 // not yet replaced
        unit.start(mark(suite), [&] {
            auto links_a = make_shared<TriangleLink>(
                Vector{19.5, 1., -.5}, Vector{19.5, 0, -1.5}, Vector{20.5, 0, -1.5});
            auto links_b = make_shared<TriangleLink>(
                Vector{19.5, 0, -1.5}, Vector{20.5, 0, -2.5}, Vector{20.5, 0, -1.5});
            auto pdriver = make_pdriver(links_a, links_b);
            Vector displacement{-0.076216, -0.00069444, -0.00069444};
            PpState state{PpOnSegment{links_a, true, Vector2{1.4142019007112767, 0.842617146393735}, Vector2{}}};
            get<PpOnSegment>(state).displacement = VtoD::find_on_segment_displacement(get<PpOnSegment>(state), displacement);

            // what should displacement be after the transfer?
            state = (*pdriver)(state, *test_handler);
            get<PpOnSegment>(state).displacement = VtoD::find_on_segment_displacement(get<PpOnSegment>(state), displacement);
            auto displc = segment_displacement_to_v3(state);
            // now I need to reverse it...

            state = (*pdriver)(state, *test_handler);

            are_very_close(displc, Vector{displacement.x, 0, displacement.z});
            return test(get<PpOnSegment>(state).fragment != links_a);
        });
        unit.start(mark(suite), [&] {
            // inverts correctly
            auto links_a = make_shared<TriangleLink>(Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 0, 1});
            auto links_b = make_shared<TriangleLink>(Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 0, -1});

            PpOnSegment a_state{links_a, true,
                links_a->segment().closest_point(Vector{ 0.5, 0, 0.1 }),
                  links_a->segment().closest_point(Vector{ 0.5, 0, -0.1 })
                - links_a->segment().closest_point(Vector{ 0.5, 0,  0.1 })};
            auto pdriver = make_pdriver(links_a, links_b);
            auto res = std::get<PpOnSegment>((*pdriver)(PpState{a_state}, *test_handler));
            return test(!res.invert_normal);
        });
#   endif
    });
    mark(suite).test([] {
        using VtoD = VelocitiesToDisplacement;
        // maybe I need to test this? -> find_on_segment_displacement
        // no this is fine...
        Vector displacement{-0.7, -0.1, -0.1};
        auto links_a = make_shared<TriangleLink>(Vector{19.5, 1., -.5}, Vector{19.5, 0, -1.5}, Vector{20.5, 0, -1.5});
        auto links_b = make_shared<TriangleLink>(Vector{19.5, 0, -1.5}, Vector{20.5, 0, -2.5}, Vector{20.5, 0, -1.5});
        const auto & a = links_a->segment();
        const auto & b = links_b->segment();
        PpOnSegment a_state{links_a, true, a.center_in_2d(), Vector2{}};
        PpOnSegment b_state{links_b, true, b.center_in_2d(), Vector2{}};
        a_state.displacement = VtoD::find_on_segment_displacement(a_state, displacement);
        b_state.displacement = VtoD::find_on_segment_displacement(b_state, displacement);
        auto a_displc = point_and_plane::segment_displacement_to_v3(PpState{a_state});
        auto b_displc = point_and_plane::segment_displacement_to_v3(PpState{b_state});
        return test(are_very_close(Vector{a_displc.x, 0, a_displc.z}, b_displc));
    });
    {
    using PCtoV = PlayerControlToVelocity;
    using PlayerMotionProfile = PCtoV::PlayerMotionProfile;
    suite.start_series("PlayerControl to Velocity");
    // Not testing camera...
    // first how can I test find_new_velocity_from_willed?
    // nah, no repeated calls

    // decellerate when no willed direction
    mark(suite).test([] {
        constexpr const Real k_et = 0.25;
        PlayerMotionProfile default_prof;
        Velocity init{5, 0, 0};
        auto after = PCtoV::find_new_velocity_from_willed(
            default_prof, init, Vector{}, k_et);
        auto decel_vel = Vector{init.value.x - default_prof.unwilled_acceleration*k_et, 0, 0};
        assert(default_prof.unwilled_acceleration*k_et < 1); // <- for the test to be valid
        return test(are_very_close(decel_vel, after.value));
    });

    // decellerate when no willed direction, but
    // do not head in the opposite direction
    mark(suite).test([] {
        constexpr const Real k_et = 3;
        PlayerMotionProfile default_prof;
        Velocity init{5, 0, 0};
        auto after = PCtoV::find_new_velocity_from_willed(
            default_prof, init, Vector{}, k_et);

        assert(default_prof.unwilled_acceleration*k_et > 1); // <- for the test to be valid
        return test(are_very_close(after, Velocity{}));
    });

    // sensible change of direction
    mark(suite).test([] {
        constexpr const Real k_et = 0.5;
        PlayerMotionProfile default_prof;
        Velocity init{5, 0, 0};
        Vector willed_dir{0, 0, 1};
        auto res = PCtoV::find_new_velocity_from_willed(
            default_prof, init, willed_dir, k_et);
        auto angl = angle_between(init.value, res.value);
        return test(angl > 0 && angl < k_pi*0.5);
    });

    // going a different direction changes velocity more than continuing in the
    // original direction
    mark(suite).test([] {
        constexpr const Real k_et = 0.5;
        PlayerMotionProfile default_prof;
        Velocity init{2, 0, 0};
        assert(magnitude(init.value) < default_prof.max_willed_speed*.5);
        auto on_straight = PCtoV::find_new_velocity_from_willed(
                    default_prof, init, Vector{1, 0, 0}, k_et).value;
        auto on_turn = PCtoV::find_new_velocity_from_willed(
                    default_prof, init, Vector{0, 0, 1}, k_et).value;
        return test(  magnitude(init.value - on_straight)
                    < magnitude(init.value - on_turn    ));
    });

    // acceleration at rest is faster, than acceleration while running
    mark(suite).test([] {
        constexpr const Real k_et = 0.25;
        PlayerMotionProfile default_prof;
        Velocity init_run{2, 0, 0};
        constexpr Vector k_willed_dir{1, 0, 0};
        auto from_rest = PCtoV::find_new_velocity_from_willed(
                    default_prof, Velocity{},  k_willed_dir, k_et).value;
        auto from_run = PCtoV::find_new_velocity_from_willed(
                    default_prof, init_run, k_willed_dir, k_et).value;
        return test(  magnitude(Vector{}       - from_rest)
                    > magnitude(init_run.value - from_run ));
    });

    // may not exceed speed cap
    mark(suite).test([] {
        constexpr const Real k_et = 0.5;
        PlayerMotionProfile default_prof;
        auto res = PCtoV::find_new_velocity_from_willed(
            default_prof, Velocity{default_prof.max_willed_speed, 0, 0},
            Vector{1, 0, 0}, k_et).value;

        return test(magnitude(res) <= default_prof.max_willed_speed);
    });

    // may slow down when beyond speed cap
    mark(suite).test([] {
        constexpr const Real k_et = 0.05;
        PlayerMotionProfile default_prof;
        Velocity init_run{Real(default_prof.max_willed_speed*1.5), 0, 0};
        auto res = PCtoV::find_new_velocity_from_willed(
            default_prof, init_run, Vector{-1, 0, 0}, k_et).value;
        // must be true, if a valid test case
        if (magnitude(res) <= default_prof.max_willed_speed) {
            throw RtError{"Test assumption failed, must result in speed faster, could also be a regular test failure."};
        }
        return test(magnitude(res) < magnitude(init_run.value));
    });

    // same direction as velocity higher than speed cap? no change
    mark(suite).test([] {
        constexpr const Real k_et = 0.5;
        PlayerMotionProfile default_prof;
        Velocity init_run{Real(default_prof.max_willed_speed*1.5), 0, 0};
        auto res = PCtoV::find_new_velocity_from_willed(
            default_prof, init_run, Vector{1, 0, 0}, k_et).value;
        return test(are_very_close(res, init_run.value));
    });

    // different direction, higher than speed cap? change direction,
    // slight decrease
    // I'm okay with this needing some opposition in order to decelerate for
    // velocity's original direction
    mark(suite).test([] {
        constexpr const Real k_et = 0.15;
        PlayerMotionProfile default_prof;
        Velocity init_run{Real(default_prof.max_willed_speed*1.5), 0, 0};
        auto res = PCtoV::find_new_velocity_from_willed(
            default_prof, init_run, normalize(Vector{-1, 0, 1}), k_et).value;
        return test(   angle_between(res, init_run.value) > 0
                    && magnitude(res) < magnitude(init_run.value));
    });

    // last note: there is no test for checking if it "feels" right short of
    // playing it

    // funky behaviors now perhaps only live... well elsewhere, with effects of
    // gravity

    // I need to test for jumping and gravitation effects

    }
#   if 0
    suite.start_series("Responses to gravity");
    // heavy down velocity, goes to zero after landing...?
    mark(suite).test([] {
        Scene scene;
        auto ent = scene.make_entity();

        constexpr const Real k_et = 0.15;
        ent.add<PpState>(PpInAir{Vector{0.5, 0.1, 0.5}, Vector{}});
        ent.add<Velocity, JumpVelocity, PlayerControl>()
            = make_tuple(Velocity{0, });
        // want a fall... land... then jump

        return test(false);
    });
#   endif
#   undef mark
    return suite.has_successes_only();
}
