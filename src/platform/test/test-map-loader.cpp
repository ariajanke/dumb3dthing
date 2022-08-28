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
#include "../../map-loader.hpp"
#include <common/TestSuite.hpp>

bool run_map_loader_tests() {
    // test movements in each cardinal direction
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using Triangle = point_and_plane::Triangle;
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("Map Loader");
    static auto load_test_layout = [] {
        static constexpr auto k_layout =
            "xxx\n"
            "xx \n"
            "x  \n";
        std::vector<Entity> entities;
        std::vector<SharedPtr<TriangleSegment>> triangles;

        TileGraphicGenerator tgg{entities, triangles, Platform::null_callbacks()};
        tgg.setup();

        auto grid = load_map_cell(k_layout, CharToCell::default_instance());
        auto gv = load_map_graphics(tgg, grid);
        return std::get<0>(gv);
    };

    using std::any_of, std::get;

    static auto any_point_arrangement_of = []
        (const Triangle & tri, const std::array<Vector, 3> & pts)
    {
        auto make_pred = [](const Vector & tri_pt) {
            return [tri_pt](const Vector & r)
                { return are_very_close(tri_pt, r); };
        };
        return    any_of(pts.begin(), pts.end(), make_pred(tri.point_a()))
               && any_of(pts.begin(), pts.end(), make_pred(tri.point_b()))
               && any_of(pts.begin(), pts.end(), make_pred(tri.point_c()));
    };

    static auto make_driver_for_test_layout = [] {
        auto links = load_test_layout();
        auto ppdriver = point_and_plane::Driver::make_driver();
        ppdriver->add_triangles(links);
        ppdriver->update();
        return ppdriver;
    };

    static auto run_intersegment_transfer = []
        (point_and_plane::Driver & driver, const Vector & start, const Vector & displacement)
    {
        PpState state = PpInAir{start + Vector{0, 0.1, 0}, Vector{0, -0.2, 0}};
        auto thandler = point_and_plane::EventHandler::make_test_handler();
        state = driver(state, *thandler);

        auto & tri = *get<PpOnSegment>(state).segment;
        get<PpOnSegment>(state).displacement =
            tri.closest_point(displacement + start) - tri.closest_point(start);

        return driver(state, *thandler);
    };

    // triangle locations sanity
    mark(suite).test([] {
        auto links = load_test_layout();
        return test(any_of(links.begin(), links.end(), [](const TriangleLinks & link) {
            const auto & tri = link.segment();
            auto pt_list = { tri.point_a(), tri.point_b(), tri.point_c() };
            return any_of(pt_list.begin(), pt_list.end(), [](Vector r)
                { return are_very_close(r.x, 2.5) && are_very_close(r.z, -2.5); });
        }));
    });

    mark(suite).test([] {
        auto ppdriver = make_driver_for_test_layout();
        PpState state = PpInAir{ Vector{2, 0.1, -1.9}, Vector{ 0, -0.2, 0 } };
        auto thandler = point_and_plane::EventHandler::make_test_handler();
        state = (*ppdriver)(state, *thandler);
        return test(get_if<PpOnSegment>(&state));
    });
    // can cross eastbound
    mark(suite).test([] {
        constexpr const Vector k_start{1.4, 0, -2};
        constexpr const Vector k_displacement{0.2, 0, 0};
        constexpr const std::array k_expect_triangle =
            { Vector{1.5, 0, -2.5}, Vector{1.5, 0, -1.5}, Vector{2.5, 0, -2.5} };

        auto ppdriver = make_driver_for_test_layout();
        auto state = run_intersegment_transfer(*ppdriver, k_start, k_displacement);

        auto & res_tri = *get<PpOnSegment>(state).segment;
        return test(any_point_arrangement_of(res_tri, k_expect_triangle));
    });
    // can cross westbound
    mark(suite).test([] {
        constexpr const Vector k_start{1.6, 0, -2};
        constexpr const Vector k_displacement{-0.2, 0, 0};
        constexpr const std::array k_expect_triangle =
            { Vector{1.5, 0, -2.5}, Vector{1.5, 0, -1.5}, Vector{0.5, 0, -1.5} };

        auto ppdriver = make_driver_for_test_layout();
        auto state = run_intersegment_transfer(*ppdriver, k_start, k_displacement);

        return test(any_point_arrangement_of(
            *get<PpOnSegment>(state).segment, k_expect_triangle));
    });
    // land on...??? which triangle in the middle?
#   undef mark
    return suite.has_successes_only();
}
