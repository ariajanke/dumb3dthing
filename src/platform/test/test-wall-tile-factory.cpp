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

#include <common/TestSuite.hpp>

#include "../../WallTileFactory.hpp"
#include "../../tiled-map-loader.hpp"

#include <tinyxml2.h>

namespace {

using Triangle = TriangleSegment;
using NeighborInfo = TileFactory::NeighborInfo;
using namespace cul::exceptions_abbr;

constexpr const auto k_flats_only          = WallTileFactory::k_flats_only;
constexpr const auto k_wall_only           = WallTileFactory::k_wall_only;
constexpr const auto k_both_flats_and_wall = WallTileFactory::k_both_flats_and_wall;

class TestTrianglesAdder final : public EntityAndTrianglesAdder {
public:
    void add_triangle(const TriangleSegment & triangle) final
        { triangles.push_back(triangle); }

    void add_entity(const Entity &) final {}

    std::vector<Triangle> triangles;
};

class WedTriangleTestAdder final : public WallTileFactory::TriangleAdder {
public:
    void operator ()(const Triangle & triangle) const final
        { triangles.push_back(triangle); }

    // I prefer this over reference
    mutable std::vector<Triangle> triangles;
};

template <typename Func>
auto make_has_wall_on_axis(Func && f) {
    return [f = std::move(f)] (const Triangle & triangle) {
        return    are_very_close(f(triangle.point_a()), f(triangle.point_b()))
               && are_very_close(f(triangle.point_b()), f(triangle.point_c()));
    };
}

template <typename Func>
auto make_array_of_components_getter(Func && f) {
    return [f = std::move(f)] (const Triangle & triangle) {
        return std::array { f(triangle.point_a()), f(triangle.point_b()), f(triangle.point_c()) };
    };
}

} // end of <anonymous> namespace

bool run_wall_tile_factory_tests() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    TestSuite suite;

    constexpr const auto k_tileset_fn = "test-tileset.tsx";
    static auto load_tileset = [](const char * fn, TileSet & tileset) {
        TiXmlDocument document;
        document.LoadFile(fn);
        auto & platform = Platform::null_callbacks();
        tileset.load_information(platform, *document.RootElement());
    };
    static constexpr const int k_connecting_tile = 16;
    static constexpr const int k_north_wall_no_translation = 34;
    static const auto k_sample_layer = [] {
        Grid<int> layer;
        layer.set_size(1, 2);
        layer(0, 0) = k_connecting_tile;
        layer(0, 1) = k_north_wall_no_translation;
        return layer;
    } ();
    static const auto make_sample_neighbor_info = [] (const TileSet & tileset) {
        return NeighborInfo{tileset, k_sample_layer, Vector2I{0, 1}, Vector2I{}};
    };
    static const auto verify_tile_factory = [] (const TileFactory * fact) {
        if (fact) return fact;
        throw RtError{"Uh Oh, no tile factory"};
    };

    suite.start_series("TileFactory::NeighborInfo");
    // no neighbor, no real number is returned
    mark(suite).test([] {
        auto res = NeighborInfo::make_no_neighbor()
            .neighbor_elevation(CardinalDirection::nw);
        return test(!cul::is_real(res));
    });
    // yes neighbor, return real number
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        auto ninfo = make_sample_neighbor_info(tileset);
        auto res = ninfo.neighbor_elevation(CardinalDirection::nw);
        return test(cul::is_real(res));
    });
    // against another wall, no real return
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        Grid<int> layer;
        layer.set_size(1, 2);
        layer(0, 0) = k_north_wall_no_translation;
        layer(0, 1) = k_north_wall_no_translation;
        NeighborInfo ninfo{tileset, layer, Vector2I{0, 1}, Vector2I{}};
        auto res = ninfo.neighbor_elevation(CardinalDirection::nw);
        return test(cul::is_real(res));
    });

    suite.start_series("Wall Tile Factory :: Triangle Generation");
    // I'd like to test different divisions here
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0, adder);

        unit.start(mark(suite), [&adder] {
            Real sum = 0;
            for (auto & triangle : adder.triangles) {
                if (!triangle.can_be_projected_onto(k_up)) continue;
                sum += triangle.project_onto_plane(k_up).area_of_triangle();
            }
            return test(are_very_close(1, sum));
        });
        unit.start(mark(suite), [&adder] {
            // I need to look for walls in the middle of the tile
            // (wall in the right spot?)
            // x ~= 0.5
            // I need something more exact...
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            if (itr == adder.triangles.end()) {
                // can't find wall
                return test(false);
            }

            return test(are_very_close(itr->point_a().z, 0));
        });
        unit.start(mark(suite), [&adder] {
            static auto get_ys = make_array_of_components_getter([] (const Vector & r) { return r.y; });
            auto tend = std::remove_if(adder.triangles.begin(), adder.triangles.end(), [](const Triangle & tri) {
                auto ys = get_ys(tri);
                return std::any_of(ys.begin(), ys.end(), [](Real y) { return !are_very_close(y, 1); });
            });
            adder.triangles.erase(tend, adder.triangles.end());
            Real sum = 0;
            for (auto & triangle : adder.triangles) {
                if (!triangle.can_be_projected_onto(k_up)) continue;
                sum += triangle.project_onto_plane(k_up).area_of_triangle();
            }
            return test(are_very_close(0.5, sum));
        });
    });
#   if 0
    mark(suite).test([] {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0, adder);
        Real sum = 0;
        for (auto & triangle : adder.triangles) {
            if (!triangle.can_be_projected_onto(k_up)) continue;
            sum += triangle.project_onto_plane(k_up).area_of_triangle();
        }
        return test(are_very_close(1, sum));
    });
    mark(suite).test([] {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0, adder);
        // I need to look for walls in the middle of the tile
        // (wall in the right spot?)
        // x ~= 0.5
        // I need something more exact...
        auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
        auto itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
        if (itr == adder.triangles.end()) {
            // can't find wall
            return test(false);
        }

        return test(are_very_close(itr->point_a().z, 0));
    });
    // same deal, find all triangles whose y is nearly 1, sum area should be ~0.5
    mark(suite).test([] {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0, adder);
        static auto get_ys = make_array_of_components_getter([] (const Vector & r) { return r.y; });
        auto tend = std::remove_if(adder.triangles.begin(), adder.triangles.end(), [](const Triangle & tri) {
            auto ys = get_ys(tri);
            return std::any_of(ys.begin(), ys.end(), [](Real y) { return !are_very_close(y, 1); });
        });
        adder.triangles.erase(tend, adder.triangles.end());
        Real sum = 0;
        for (auto & triangle : adder.triangles) {
            if (!triangle.can_be_projected_onto(k_up)) continue;
            sum += triangle.project_onto_plane(k_up).area_of_triangle();
        }
        return test(are_very_close(0.5, sum));
    });
#   endif
    // next, corner wall tiles
    // test if symmetry of imlpementation is okay
    // one for wall, make sure wall is there, flat is correct location, wall
    // is in the right spot
    // symmetry of corners, walls there, flat area okay, (and also right place)

    suite.start_series("Wall Tile Factory");
    mark(suite).test([] {
        // not possible without neighbors!
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        using Wtf = WallTileFactory;
        using Cd = CardinalDirection;

        auto wed = Wtf::elevations_and_direction
            (make_sample_neighbor_info(tileset), 1, Cd::n, Vector2I{});
        // both dip heights should be 2 in this case
        auto nw = wed.dip_heights[Wtf::corner_index(Cd::nw)];
        auto ne = wed.dip_heights[Wtf::corner_index(Cd::ne)];
        auto sw = wed.dip_heights[Wtf::corner_index(Cd::sw)];
        auto se = wed.dip_heights[Wtf::corner_index(Cd::se)];
        return test(   are_very_close(ne, 1) && are_very_close(nw, 1)
                    && are_very_close(sw, 0) && are_very_close(se, 0));
    });

    // if there are no neighbors, then no walls should be generated!
    mark(suite).test([] {
        using Wtf = WallTileFactory;
        using Cd = CardinalDirection;

        auto wed = Wtf::elevations_and_direction
            (NeighborInfo::make_no_neighbor(), 1, Cd::n, Vector2I{});
        auto nw = wed.dip_heights[Wtf::corner_index(Cd::nw)];
        auto ne = wed.dip_heights[Wtf::corner_index(Cd::ne)];
        auto sw = wed.dip_heights[Wtf::corner_index(Cd::sw)];
        auto se = wed.dip_heights[Wtf::corner_index(Cd::se)];
        return test(   are_very_close(ne, 0) && are_very_close(nw, 0)
                    && are_very_close(sw, 0) && are_very_close(se, 0));
    });
    static const auto all_equal_zs = [](const Triangle & tptr) {
        return    are_very_close(tptr.point_a().z, tptr.point_b().z)
               && are_very_close(tptr.point_b().z, tptr.point_c().z);
    };
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        auto * wall_tile_factory = verify_tile_factory(tileset(k_north_wall_no_translation));

        TestTrianglesAdder adder;
        // what the tile is, appears through the adder

        auto sample_neighbor = make_sample_neighbor_info(tileset);
        (*wall_tile_factory)(adder, sample_neighbor, Platform::null_callbacks());
        bool wall_found = std::any_of(adder.triangles.begin(), adder.triangles.end(),
            all_equal_zs);
        return test(wall_found);
    });
    // it *is* correct behavior to not generate walls when there is no neighbor
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        auto * wall_tile_factory = verify_tile_factory(tileset(k_north_wall_no_translation));

        TestTrianglesAdder adder;
        // what the tile is, appears through the adder

        auto sample_neighbor = NeighborInfo::make_no_neighbor();
        (*wall_tile_factory)(adder, sample_neighbor, Platform::null_callbacks());
        bool wall_not_found = std::none_of(adder.triangles.begin(), adder.triangles.end(),
            all_equal_zs);
        return test(wall_not_found);
    });

    // I'd like test cases for...
    // corners
    // mid splits (where the wall is in the middle of the tile)
#   undef mark
    return suite.has_successes_only();
}
