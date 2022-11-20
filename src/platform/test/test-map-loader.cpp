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
#include "../../map-loader/map-loader.hpp"
#include "../../map-loader/tiled-map-loader.hpp"
#include "../../PointAndPlaneDriver.hpp"

#include <ariajanke/cul/TestSuite.hpp>

#include <cstring>

#include <fstream>
#include <sstream>

namespace {

#define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE

using PlayerEntities = LoaderTask::PlayerEntities;
using namespace cul::exceptions_abbr;
using cul::is_real;
using LinksGrid = Grid<View<TriangleLinks::const_iterator, TriangleLinks::const_iterator>>;
using Triangle = TriangleSegment;

class TestLoaderTaskCallbacks final : public LoaderTask::Callbacks {
public:
    void add(const SharedPtr<EveryFrameTask> &) final {}

    void add(const SharedPtr<OccasionalTask> &) final {}

    void add(const SharedPtr<LoaderTask> &) final {};

    void add(const Entity & e) final { entities.push_back(e); }

    Platform & platform() final
        { return Platform::null_callbacks(); }

    PlayerEntities player_entites() const final
        { return PlayerEntities{}; }

    void set_player_entities(const PlayerEntities &) final {}

    std::vector<Entity> entities;
};

bool run_tiled_map_loader_tests();

} // end of <anonymous> namespace

bool run_map_loader_tests() {
    return run_tiled_map_loader_tests();
}

namespace {

class PlatformWithFileSystem final : public Platform {
public:

    static PlatformWithFileSystem & isntance() {
        static PlatformWithFileSystem inst;
        return inst;
    }

    void render_scene(const Scene &) final {}

    Entity make_renderable_entity() const final
        { return Platform::null_callbacks().make_renderable_entity(); }

    SharedPtr<Texture> make_texture() const final
        { return Platform::null_callbacks().make_texture(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return Platform::null_callbacks().make_render_model(); }

    void set_camera_entity(EntityRef) final {}

    FutureStringPtr promise_file_contents(const char * filename) {
        class Impl final : public Future<std::string> {
        public:
            explicit Impl(const char * filename):
                m_str(file_to_string(filename)) {}

            bool is_ready() const noexcept final { return true; }

            bool is_lost() const noexcept final { return false; }

            std::string && retrieve() final { return std::move(m_str); };
        private:
            std::string m_str;
        };
        return make_unique<Impl>(filename);
    }

private:
    // this is copied from another location
    // presently I don't have a good solution to "share" this between platforms
    static std::string file_to_string(const char * filename) {
        std::ifstream fin;
        fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fin.open(filename);
        std::stringstream sstrm;
        sstrm << fin.rdbuf();
        fin.close();
        return sstrm.str();
    }


    PlatformWithFileSystem() {}
};

bool run_tiled_map_loader_tests() {
    using namespace cul::ts;
    using MapLinks = MapEdgeLinks::MapLinks;
    using TileRange = MapEdgeLinks::TileRange;

    TestSuite suite;
    suite.start_series("Tiled Map Loader");
    // for these test cases, I'm going to do something wild:
    // I'm going to use "fixtures"

    // bin/test-map-links.tmx:
    static constexpr const auto k_map_links_fn = "test-map-links.tmx";
    static constexpr const auto k_walls_corner_fn = "test-walls-corner.tmx";
    static auto get_preparing_loader = [] (const char * fn) {
        MapLoader loader{PlatformWithFileSystem::isntance()};
        loader.start_preparing(fn);
        return loader;
    };
    // also verifies that it's actually gotten.. (no nullptr)
    static auto check_load_task = [] (MapLoader & loader, Vector2I offset) {
        auto loadtask = std::get<0>(loader(offset));
        if (!loadtask) { throw RtError{"???"}; }
        return loadtask;
    };

    static auto do_load_task_get_entities = [] (MapLoader & loader, Vector2I offset) {
        auto loadtask = check_load_task(loader, offset);
        TestLoaderTaskCallbacks test_callbacks;
        (*loadtask)(test_callbacks);
        return test_callbacks.entities;
    };

    static auto elevation_for_all = [](const LinksGrid::Element & el, Real y) {
        static_assert(std::is_same_v<LinksGrid::Element, View<TriangleLinks::const_iterator>>);
        return std::all_of(el.begin(), el.end(), [y](const SharedPtr<TriangleLink> & links) {
            const auto & tri = links->segment();
            auto list = { tri.point_a().y, tri.point_b().y, tri.point_c().y };
            return std::all_of(list.begin(), list.end(),
                               [y](Real x) { return are_very_close(x, y); });
        });
    };

    // correct elevation
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_map_links_fn);
        auto entities = do_load_task_get_entities(loader, Vector2I{});

        // I *really* want a better way to address the tiles than just having
        // to get them all at once. The load task though is intented to get
        // entities for the scene.

        // This current design is sort of a "trick". The driver/scheduler
        // should not handle anything other than entities.

        const auto & links = entities.back().get<TriangleLinks>();
        // tell me these links look okay...
        // (note values are fixture specific)
        return test(std::all_of(links.begin(), links.end(), [] (const SharedPtr<TriangleLink> & link) {
            auto list = { link->segment().point_a().y, link->segment().point_b().y, link->segment().point_c().y };
            return std::all_of(list.begin(), list.end(), [](Real y) { return are_very_close(y, 0); });
        }));
    });

    // links sanity test
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_map_links_fn);
        (void)do_load_task_get_entities(loader, Vector2I{});
        const auto & [filename, tile_range] = *loader.northern_maps().begin(); {}
        return test(   ::strcmp(filename.c_str(), k_map_links_fn) == 0
                    && tile_range == TileRange{Vector2I{}, Vector2I{loader.width(), 0}});
    });

    // I want to try... another side with ranges...
    // what should the value look like? "whole" is nice for everything
    // but what about ranges?
    mark(suite).test([] {
        using std::get;
        // south
        auto loader = get_preparing_loader(k_map_links_fn);
        (void)do_load_task_get_entities(loader, Vector2I{});
        auto south_maps = loader.southern_maps();
        // it would be nice to do mupltiple assertions in a test right?
        bool names_good = std::all_of(south_maps.begin(), south_maps.end(),
            [] (const MapLinks & links)
            { return links.filename == k_map_links_fn; });
        return test(
               south_maps.begin()[0].range == TileRange{Vector2I{0, 2}, Vector2I{1, 2}}
            && south_maps.begin()[1].range == TileRange{Vector2I{1, 2}, Vector2I{2, 2}}
            && names_good);
    });

    // generally conflicts may occur at any point of the "link" tree
    // The MapLoader does not handle conflicts

    // verify that "offsetting" works from another side
    // also helps complete coverage for all sides
    mark(suite).test([] {
        // east
        auto loader = get_preparing_loader(k_map_links_fn);
        (void)do_load_task_get_entities(loader, Vector2I{});
        auto [filename, tile_range] = *loader.eastern_maps().begin(); {}
        constexpr const Vector2I k_offset{-20, 10};
        tile_range = tile_range + k_offset;
        return test(   tile_range.begin_location() == k_offset + Vector2I{2, 0}
                    && tile_range.end_location  () == k_offset + Vector2I{2, 2});
    });

    // on yet another side, multiple map arguments should be possible
    mark(suite).test([] {
        // west
        using std::get;
        auto loader = get_preparing_loader(k_map_links_fn);
        (void)do_load_task_get_entities(loader, Vector2I{});
        auto west_maps = loader.western_maps();
        auto west0 = west_maps.begin()[0];
        auto west1 = west_maps.begin()[1];
        return test(   west0.range == TileRange{Vector2I{0, 0}, Vector2I{0, 1}}
                    && west1.range == TileRange{Vector2I{0, 1}, Vector2I{0, 2}}
                    && west0.filename == k_map_links_fn
                    && west1.filename == k_walls_corner_fn);
    });

    // bin/test-walls-elevations.tmx
    constexpr const auto k_walls_elv_map = "test-walls-elevations.tmx";

    // check for flat in map (and that it's correct)
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_walls_elv_map);
        auto entities = do_load_task_get_entities(loader, Vector2I{});
        // everything that grid points to should be owned by that last entity
        const auto & grid = entities.back().get<LinksGrid>();
        bool floor_tile_at_3 = elevation_for_all(grid(Vector2I{1, 1}), 2);
        return test(floor_tile_at_3);
    });

    // search for flats that fills a whole square's worth
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_walls_elv_map);
        auto entities = do_load_task_get_entities(loader, Vector2I{});
        // everything that grid points to should be owned by that last entity
        const auto & grid = entities.back().get<LinksGrid>();

        Real sum = 0;
        for (auto & link : grid(Vector2I{0, 1})) {
            auto & tri = link->segment();
            if (!tri.can_be_projected_onto(k_up))
                continue;
            sum += tri.project_onto_plane(k_up).area();
        }

        return test(are_very_close(sum, 1));
    });


    // is a (specific) wall generated?
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_walls_elv_map);
        auto entities = do_load_task_get_entities(loader, Vector2I{});

        // everything that grid points to should be owned by that last entity
        const auto & grid = entities.back().get<LinksGrid>();
        auto wall_tile = grid(Vector2I{0, 1});
        bool wall_found = std::any_of(wall_tile.begin(), wall_tile.end(),
            [](const SharedPtr<TriangleLink> & links)
        {
            // all the same x, where x == 1
            auto tri = links->segment();
            return    are_very_close(tri.point_a().x, tri.point_b().x)
                   && are_very_close(tri.point_b().x, tri.point_c().x);
        });
        return test(wall_found);
    });

    // do walls of different elevation work?
    mark(suite).test([] {
        auto loader = get_preparing_loader(k_walls_elv_map);
        auto entities = do_load_task_get_entities(loader, Vector2I{});

        // everything that grid points to should be owned by that last entity
        // pointers go bad?!
        const auto & grid = entities.back().get<LinksGrid>();
        auto wall_tile = grid(Vector2I{0, 0});

        Real high_y = -k_inf;
        Real low_y = k_inf;
        // get the greatest height difference, expect it to be about 2
        for (auto & links : wall_tile) {
            auto & tri = links->segment();
            if (   are_very_close(tri.point_a().x, tri.point_b().x)
                && are_very_close(tri.point_b().x, tri.point_c().x))
            {
                for (auto y : { tri.point_a().y, tri.point_b().y, tri.point_c().y }) {
                    high_y = std::max(high_y, y);
                    low_y  = std::min(low_y , y);
                }
            }
        }
        return test(   is_real(high_y) && is_real(low_y)
                    && are_very_close(high_y - low_y, 2));
    });
#   if 0
    // special case, other tile's elevation is the same
    mark(suite).test([] {
        return test(false);
    });
    // special case, two walls face each other (different elevations)
    mark(suite).test([] {
        return test(false);
    });
    // bin/test-walls-sloped-corners.tmx
    // does the n/s wall look good?
    mark(suite).test([] {
        return test(false);
    });
    // does the e/w wall look good?
    mark(suite).test([] {
        return test(false);
    });
#   endif
    return suite.has_successes_only();
}

} // end of <anonymous> namespace
