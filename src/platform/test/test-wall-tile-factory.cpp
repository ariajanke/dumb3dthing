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

#include <ariajanke/cul/TestSuite.hpp>

#include "../../map-loader/TileSet.hpp"
#include "../../map-loader/WallTileFactory.hpp"

#include <tinyxml2.h>

namespace {

using Triangle = TriangleSegment;
using namespace cul::exceptions_abbr;

constexpr const auto k_flats_only          = WallTileFactoryBase::k_bottom_only | WallTileFactoryBase::k_top_only;
constexpr const auto k_wall_only           = WallTileFactoryBase::k_wall_only;
constexpr const auto k_both_flats_and_wall = WallTileFactoryBase::k_both_flats_and_wall;

class TestTrianglesAdder final : public EntityAndTrianglesAdder {
public:
    void add_triangle(const TriangleSegment & triangle) final
        { triangles.push_back(triangle); }

    void add_entity(const Entity &) final {}

    std::vector<Triangle> triangles;
};

class WedTriangleTestAdder final : public TriangleAdder {
public:
    void operator () (const Triangle & triangle) const final
        { triangles.push_back(triangle); }

    auto begin() { return triangles.begin(); }

    auto end() { return triangles.end(); }

    // I prefer this over reference
    mutable std::vector<Triangle> triangles;
};

class SingleTileSetGrid final : public SlopesGridInterface {
public:
    SingleTileSetGrid(const TileSet & ts, const Grid<int> & idgrid):
        m_tileset(ts),
        m_grid(idgrid)
    {}

    Slopes operator () (Vector2I r) const final {
        if (!m_grid.has_position(r))
            return Slopes{k_inf, k_inf, k_inf, k_inf};
        auto factory = m_tileset(m_grid(r));
        if (!factory)
            return Slopes{k_inf, k_inf, k_inf, k_inf};
        return factory->tile_elevations();
    }

    NeighborInfo make_neighbor_info(Vector2I r) const
        { return NeighborInfo{*this, r, Vector2I{}}; }

private:
    const TileSet & m_tileset;
    const Grid<int> & m_grid;
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

Real sum_of_areas(const std::vector<Triangle> & triangles) {
    Real sum = 0;
    std::vector<Triangle> projected_triangles;
    for (auto & triangle : triangles) {
        if (!triangle.can_be_projected_onto(k_up)) continue;
        // | will cause tests to fail without projection
        // | but that's okay, non-projected flats shouldn't exceed their
        // v expected sizes
        sum += triangle.area();
        projected_triangles.push_back(triangle.project_onto_plane(k_up));
    }
    return sum;
}

void remove_non_top_flats(std::vector<Triangle> & triangles) {
    static auto get_ys = make_array_of_components_getter([] (const Vector & r) { return r.y; });
    auto tend = std::remove_if(triangles.begin(), triangles.end(), [](const Triangle & tri) {
        auto ys = get_ys(tri);
        return std::any_of(ys.begin(), ys.end(), [](Real y) { return !are_very_close(y, 1); });
    });
    triangles.erase(tend, triangles.end());
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
        tileset.load(platform, *document.RootElement());
    };
    static constexpr const int k_connecting_tile = 16;
    static constexpr const int k_north_wall_no_translation = 34;
    static constexpr const int k_south_wall_no_translation = 52;
    static constexpr const int k_east_wall_no_translation  = 44;
    static constexpr const int k_nw_wall = 33;
    static constexpr const int k_ne_wall = 35;
    static constexpr const int k_se_wall = 53;
    static const auto k_sample_layer = [] {
        Grid<int> layer;
        layer.set_size(1, 2);
        layer(0, 0) = k_connecting_tile;
        layer(0, 1) = k_north_wall_no_translation;
        return layer;
    } ();
    static const auto make_sample_map_grid = [](const TileSet & tileset) {
        return SingleTileSetGrid{tileset, k_sample_layer};
    };
#   if 0
    static const auto make_sample_neighbor_info = [] (const TileSet & tileset) {
        return NeighborInfo{tileset, k_sample_layer, Vector2I{0, 1}, Vector2I{}};
    };
#   endif
    static const auto verify_tile_factory = [] (const TileFactory * fact) {
        if (fact) return fact;
        throw RtError{"Uh Oh, no tile factory"};
    };

    suite.start_series("TileFactory :: NeighborInfo");
    // fundemental problem with neighbor info

#   if 0
    // no neighbor, no real number is returned
    mark(suite).test([] {
        auto res = NeighborInfo::make_no_neighbor()
            .neighbor_elevation(CardinalDirection::nw);
        return test(!cul::is_real(res));
    });
#   endif
    // yes neighbor, return real number
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        auto mapgrid = make_sample_map_grid(tileset);
        auto ninfo = mapgrid.make_neighbor_info(Vector2I{0, 1});
#       if 0
        //make_sample_neighbor_info(tileset);
#       endif
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
        SingleTileSetGrid mapgrid{tileset, layer};
#       if 0
        NeighborInfo ninfo{tileset, layer, Vector2I{0, 1}, Vector2I{}};
#       endif
        auto ninfo = mapgrid.make_neighbor_info(Vector2I{0, 1});
        auto res = ninfo.neighbor_elevation(CardinalDirection::nw);
        return test(cul::is_real(res));
    });
#   if 0
    // elevations (dip heights) are okay for sample neighbor
    mark(suite).test([] {
        using Wtf = WallTileFactoryBase;
        using Cd = CardinalDirection;
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        auto mapgrid = make_sample_map_grid(tileset);
        auto ninfo = mapgrid.make_neighbor_info(Vector2I{0, 1});
#       if 0
                make_sample_neighbor_info(tileset);
#       endif
        auto wed = WallTileGraphicKey; Wtf::elevations_and_direction(ninfo, 1, CardinalDirection::n, Vector2I{0, 1});
        auto nw = wed.dip_heights[Wtf::corner_index(Cd::nw)];
        auto ne = wed.dip_heights[Wtf::corner_index(Cd::ne)];
        auto sw = wed.dip_heights[Wtf::corner_index(Cd::sw)];
        auto se = wed.dip_heights[Wtf::corner_index(Cd::se)];
        return test(   are_very_close(nw, 1) && are_very_close(ne, 1)
                    && are_very_close(sw, 0) && are_very_close(se, 0));
    });

    // elevations and dips for an east wall
    mark(suite).test([] {
        using Wtf = WallTileFactory;
        using Cd = CardinalDirection;
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);

        Grid<int> layer;
        layer.set_size(2, 1);
        layer(0, 0) = k_east_wall_no_translation;
        layer(1, 0) = k_connecting_tile;
        SingleTileSetGrid mapgrid{tileset, layer};
#       if 0
        auto ninfo = TileFactory::NeighborInfo{tileset, layer, Vector2I{}, Vector2I{}};
#       endif
        auto ninfo = mapgrid.make_neighbor_info(Vector2I{});
        auto wed = Wtf::elevations_and_direction(ninfo, 1, CardinalDirection::e, Vector2I{0, 1});
        auto nw = wed.dip_heights[Wtf::corner_index(Cd::nw)];
        auto ne = wed.dip_heights[Wtf::corner_index(Cd::ne)];
        auto sw = wed.dip_heights[Wtf::corner_index(Cd::sw)];
        auto se = wed.dip_heights[Wtf::corner_index(Cd::se)];
        return test(   are_very_close(nw, 0) && are_very_close(ne, 1)
                    && are_very_close(sw, 0) && are_very_close(se, 1));
    });

    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        using Cd = CardinalDirection;
        using Wtf = WallTileFactory;
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);

        Grid<int> layer;
        layer.set_size(1, 2);
        layer(0, 0) = k_south_wall_no_translation;
        layer(0, 1) = k_connecting_tile;
        SingleTileSetGrid mapgrid{tileset, layer};

        auto ninfo = mapgrid.make_neighbor_info(Vector2I{});
#       if 0
        TileFactory::NeighborInfo{tileset, layer, Vector2I{}, Vector2I{}};
#       endif
        // catch bug in neighbor elevations
        unit.start(mark(suite), [&ninfo] {
            return test(cul::is_real(ninfo.neighbor_elevation(Cd::se)));
        });

        unit.start(mark(suite), [&ninfo] {
            auto wed = Wtf::elevations_and_direction(ninfo, 1, CardinalDirection::s, Vector2I{0, 1});
            auto nw = wed.dip_heights[Wtf::corner_index(Cd::nw)];
            auto ne = wed.dip_heights[Wtf::corner_index(Cd::ne)];
            auto sw = wed.dip_heights[Wtf::corner_index(Cd::sw)];
            auto se = wed.dip_heights[Wtf::corner_index(Cd::se)];
            return test(   are_very_close(nw, 0) && are_very_close(ne, 0)
                        && are_very_close(sw, 1) && are_very_close(se, 1));
        });
    });
#   endif
#   if 0
    suite.start_series("Wall Tile Factory :: Triangle Generation");
    // I'd like to test different divisions here
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder triangles;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0, triangles);

        // sum of flats ~1
        unit.start(mark(suite), [&triangles] {
            return test(are_very_close(1, sum_of_areas(triangles.triangles)));
        });
        unit.start(mark(suite), [&triangles] {
            // I need to look for walls in the middle of the tile
            // (wall in the right spot?)
            // x ~= 0.5
            // I need something more exact...
            auto is_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto itr = std::find_if(triangles.begin(), triangles.end(), is_wall_on_z);
            if (itr == triangles.end()) {
                // can't find wall
                return test(false);
            }

            return test(are_very_close(itr->point_a().z, 0));
        });
        unit.start(mark(suite), [&triangles] {
            remove_non_top_flats(triangles.triangles);
            return test(are_very_close(0.5, sum_of_areas(triangles.triangles)));
        });
    });

    mark(suite).test([] {
        WedTriangleTestAdder triangles;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::n, 0, 1, 1, 0, k_both_flats_and_wall, 0.25, triangles);
        remove_non_top_flats(triangles.triangles);
        return test(are_very_close(0.25, sum_of_areas(triangles.triangles)));
    });

    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder triangles;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::e, 1, 1, 0, 0, k_both_flats_and_wall, 0.25, triangles);

        // sum of flats ~1
        unit.start(mark(suite), [&triangles] {
            return test(are_very_close(1, sum_of_areas(triangles.triangles)));
        });
        unit.start(mark(suite), [&triangles] {
            auto is_wall_on_x = make_has_wall_on_axis([](const Vector & r)
                { return r.x; });
            auto itr = std::find_if(triangles.triangles.begin(), triangles.triangles.end(), is_wall_on_x);
            if (itr == triangles.triangles.end()) {
                // can't find wall
                return test(false);
            }

            return test(are_very_close(itr->point_a().x, -0.25));
        });
        unit.start(mark(suite), [&triangles] {
            remove_non_top_flats(triangles.triangles);
            return test(are_very_close(0.25, sum_of_areas(triangles.triangles)));
        });
        // is a top generally in the right place?
        unit.start(mark(suite), [&triangles] {
            remove_non_top_flats(triangles.triangles);
            static constexpr const Real k_eastmost = -0.5;
            static constexpr const Real k_westmost = -0.25;
            bool eastmost_found = false;
            bool westmost_found = false;
            for (const auto & triangle : triangles) {
                auto xs = { triangle.point_a().x, triangle.point_b().x, triangle.point_c().x };
                auto make_very_close_to = [](Real xt) {
                    return [xt](Real x) { return are_very_close(xt, x); };
                };

                eastmost_found |= std::any_of(xs.begin(), xs.end(), make_very_close_to(k_eastmost));
                westmost_found |= std::any_of(xs.begin(), xs.end(), make_very_close_to(k_westmost));
            }
            return test(eastmost_found && westmost_found);
        });
    });


    // next, corner wall tiles
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder adder;
        // the one goes in the opposite corner of the corner given
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::nw, 0, 0, 1, 0, k_both_flats_and_wall, -0.25, adder);
        // total area okay
        unit.start(mark(suite), [&adder] {
            return test(are_very_close(1, sum_of_areas(adder.triangles)));
        });
        // walls, along both axises
        unit.start(mark(suite), [&adder] {
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto ew_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            auto has_wall_on_x = make_has_wall_on_axis([](const Vector & r) { return r.x; });
            auto ns_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_x);
            return test(   ew_itr != adder.triangles.end()
                        && ns_itr != adder.triangles.end());
        });
        // walls, in the right place
        unit.start(mark(suite), [&adder] {
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto ew_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            auto has_wall_on_x = make_has_wall_on_axis([](const Vector & r) { return r.x; });
            auto ns_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_x);
            if (   ew_itr == adder.triangles.end()
                || ns_itr == adder.triangles.end())
            { return test(false); }
            return test(   are_very_close(ew_itr->point_a().z,  0.25)
                        && are_very_close(ns_itr->point_a().x, -0.25));
        });
        // total area of top okay
        unit.start(mark(suite), [&adder] {
            remove_non_top_flats(adder.triangles);
            return test(are_very_close(0.75*0.75, sum_of_areas(adder.triangles)));
        });
    });
    // test if symmetry of imlpementation is okay
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::se, 1, 0, 0, 0, k_both_flats_and_wall, 0.25, adder);
        // total area okay
        unit.start(mark(suite), [&adder] {
            return test(are_very_close(1, sum_of_areas(adder.triangles)));
        });
        // walls, in the right place
        unit.start(mark(suite), [&adder] {
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto ew_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            auto has_wall_on_x = make_has_wall_on_axis([](const Vector & r) { return r.x; });
            auto ns_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_x);
            if (   ew_itr == adder.triangles.end()
                || ns_itr == adder.triangles.end())
            { return test(false); }
            return test(   are_very_close(ew_itr->point_a().z,  0.25)
                        && are_very_close(ns_itr->point_a().x, -0.25));
        });
        // total area of top okay
        unit.start(mark(suite), [&adder] {
            remove_non_top_flats(adder.triangles);
            return test(are_very_close(0.25*0.25, sum_of_areas(adder.triangles)));
        });
    });

    // ahhh ne, and sw now
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::ne, 0, 1, 0, 0, k_both_flats_and_wall, 0.25, adder);
        // walls, in the right place
        unit.start(mark(suite), [&adder] {
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto ew_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            auto has_wall_on_x = make_has_wall_on_axis([](const Vector & r) { return r.x; });
            auto ns_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_x);
            if (   ew_itr == adder.triangles.end()
                || ns_itr == adder.triangles.end())
            { return test(false); }
            return test(   are_very_close(ew_itr->point_a().z, -0.25)
                        && are_very_close(ns_itr->point_a().x, -0.25));
        });
        // total area of top okay
        unit.start(mark(suite), [&adder] {
            remove_non_top_flats(adder.triangles);
            return test(are_very_close(0.25*0.25, sum_of_areas(adder.triangles)));
        });
    });

    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        WedTriangleTestAdder adder;
        WallTileFactory::add_wall_triangles_to
            (CardinalDirection::sw, 0, 0, 0, 1, k_both_flats_and_wall, -0.25, adder);
        // walls, in the right place
        unit.start(mark(suite), [&adder] {
            auto has_wall_on_z = make_has_wall_on_axis([](const Vector & r) { return r.z; });
            auto ew_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_z);
            auto has_wall_on_x = make_has_wall_on_axis([](const Vector & r) { return r.x; });
            auto ns_itr = std::find_if(adder.triangles.begin(), adder.triangles.end(), has_wall_on_x);
            if (   ew_itr == adder.triangles.end()
                || ns_itr == adder.triangles.end())
            { return test(false); }
            return test(   are_very_close(ew_itr->point_a().z, -0.25)
                        && are_very_close(ns_itr->point_a().x, -0.25));
        });
        // total area of top okay
        unit.start(mark(suite), [&adder] {
            remove_non_top_flats(adder.triangles);
            return test(are_very_close(0.75*0.75, sum_of_areas(adder.triangles)));
        });
    });

    suite.start_series("Wall Tile Factory");
    mark(suite).test([] {
        // not possible without neighbors!
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        using Wtf = WallTileFactory;
        using Cd = CardinalDirection;
        auto mapgrid = make_sample_map_grid(tileset);
        auto wed = Wtf::elevations_and_direction
            (mapgrid.make_neighbor_info(Vector2I{0, 1}), 1, Cd::n, Vector2I{});
#       if 0
        auto wed = Wtf::elevations_and_direction
            (make_sample_neighbor_info(tileset), 1, Cd::n, Vector2I{});
#       endif
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
        auto mapgrid = make_sample_map_grid(tileset);
        auto sample_neighbor = mapgrid.make_neighbor_info(Vector2I{0, 1});
#       if 0
                make_sample_neighbor_info(tileset);
#       endif
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

    // three corners face each other, elevation becomes unknown
    mark(suite).test([] {
        TileSet tileset;
        load_tileset(k_tileset_fn, tileset);
        Grid<int> layer;
        layer.set_size(2, 2);
        layer(0, 0) = k_se_wall;
        layer(1, 1) = k_nw_wall;
        layer(0, 1) = k_ne_wall; // <- target
        auto mapgrid = make_sample_map_grid(tileset);
        auto ninfo = mapgrid.make_neighbor_info(Vector2I{0, 1});
#       if 0
        NeighborInfo ninfo{tileset, layer, Vector2I{0, 1}, Vector2I{}};
#       endif
        auto * ne_factory = tileset(k_ne_wall);
        if (!ne_factory) {
            throw RtError{"No tile factory?"};
        }
        // need neighbor elevations
        auto res = ninfo.neighbor_elevation(CardinalDirection::ne);
        return test(!cul::is_real(res));
    });
    // I'd like test cases for...
    // corners
    // mid splits (where the wall is in the middle of the tile)
#   endif
#   undef mark
    return suite.has_successes_only();
}
