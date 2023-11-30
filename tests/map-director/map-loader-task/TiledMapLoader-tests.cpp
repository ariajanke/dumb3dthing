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

#include "../../../src/map-director/map-loader-task/TiledMapLoader.hpp"

#include "../../test-helpers.hpp"

#include <tinyxml2.h>

#include <set>

namespace {

using namespace cul::tree_ts;

constexpr auto k_test_map_content =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<map version=\"1.8\" tiledversion=\"1.8.0\" orientation=\"orthogonal\" "
         "renderorder=\"right-down\" width=\"2\" height=\"2\" tilewidth=\"32\" "
         "tileheight=\"32\" infinite=\"0\" nextlayerid=\"2\" nextobjectid=\"1\">"
    "<tileset firstgid=\"1\" name=\"test-tileset\" tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
      "<image source=\"test-tileset.png\" width=\"32\" height=\"32\"/>"
      "<tile id=\"0\" type=\"test-tile-type\"></tile>"
    "</tileset>"
    "<layer id=\"1\" name=\"Tile Layer 1\" width=\"2\" height=\"2\">"
      "<data encoding=\"csv\">1,1,1,1</data>"
    "</layer>"
    "</map>";

class TestTileset final : public TilesetBase {
public:
    Continuation & load
        (const TiXmlElement &, MapContentLoader &) final
        { throw ""; }

    void add_map_elements
        (TilesetMapElementCollector &, const TilesetLayerWrapper &) const final
        { throw ""; }

private:
    Size2I size2() const final { return Size2I{1, 1}; }
};

using StartGidWithTileset = TileMapIdToSetMapping::StartGidWithTileset;

struct Info final {
    TilesetBase * tileset = nullptr;
    std::vector<TilesetMappingTile> mapping_tiles;
};

class TestProducableTile final : public ProducableTile {
public:
    TestProducableTile() {}

    TestProducableTile(Vector2I on_map_, Vector2I on_tileset_):
        on_map(on_map_), on_tileset(on_tileset_) {}

    void operator () (ProducableTileCallbacks &) const final {}

    Vector2I on_map, on_tileset;
};

class TestProducableGroupFiller final : public ProducableGroupFiller {
public:
    static TestProducableGroupFiller & instance() {
        return *instance_ptr();
    }

    static SharedPtr<TestProducableGroupFiller> instance_ptr() {
        static auto inst = make_shared<TestProducableGroupFiller>();
        return inst;
    }


    ProducableGroupTileLayer operator ()
        (const std::vector<TileLocation> & tile_locations,
         ProducableGroupTileLayer && layer) const
    {
        UnfinishedProducableGroup<TestProducableTile> group;
        for (auto & loc : tile_locations) {
            group.at_location(loc.on_map).make_producable(loc.on_map, loc.on_tileset);
        }
        layer.add_group(std::move(group));
        return std::move(layer);
    }
};

class TestMapContentLoader final : public MapContentLoader {
public:
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;
    static constexpr const char * k_test_map = "blah";

    static TestMapContentLoader & instance() {
        static TestMapContentLoader inst;
        return inst;
    }

    static ContinuationStrategy & continuation_strategy() {
        using Continuation = BackgroundTask::Continuation;
        class ContImpl final : public Continuation {
        public:
            Continuation & wait_on(const SharedPtr<BackgroundTask> & task) {
                instance().wait_on(task);
                return *this;
            }
        };

        class Impl final : public ContinuationStrategy {
        public:
            Continuation & continue_() { return impl; }

        private:
            ContImpl impl;
        };
        static Impl impl;
        return impl;
    }

    SharedPtr<Texture> make_texture() const final
        { return Platform::null_callbacks().make_texture(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return Platform::null_callbacks().make_render_model(); }

    FutureStringPtr promise_file_contents(const char * fn) final {
        if (::strcmp(fn, k_test_map))
            { throw "unhandled"; }
        class Impl final : public Future<std::string> {
            OptionalEither<Lost, std::string> retrieve()
                { return std::string{k_test_map_content}; }
        };
        return make_shared<Impl>();
    }

    const FillerFactoryMap & map_fillers() const final {
        static FillerFactoryMap map = [] {
            FillerFactoryMap map;
            map["test-tile-type"] = []
                (const TilesetXmlGrid &, PlatformAssetsStrategy &)
                    -> SharedPtr<ProducableGroupFiller>
                { return TestProducableGroupFiller::instance_ptr(); };
            return map;
        } ();
        return map;
    }

    bool delay_required() const final
        { return !waited_on_tasks.empty(); }

    void add_warning(MapLoadingWarningEnum) final
        { throw "unhandled"; }

    void wait_on(const SharedPtr<BackgroundTask> & task) final
        { waited_on_tasks.push_back(task); }

    TaskContinuation & task_continuation() const final
        { throw "unhandled"; }

    std::vector<SharedPtr<BackgroundTask>> waited_on_tasks;
};

class TestPlatform final : public Platform {
public:
    static TestPlatform & instance() {
        static TestPlatform inst;
        return inst;
    }

    SharedPtr<Texture> make_texture() const final
        { return TestMapContentLoader::instance().make_texture(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return TestMapContentLoader::instance().make_render_model(); }

    FutureStringPtr promise_file_contents(const char * fn) final {
        return TestMapContentLoader::instance().promise_file_contents(fn);
    }

    void render_scene(const Scene &) final { throw "unhandled"; }

    Entity make_renderable_entity() const final { throw "unhandled"; }

    void set_camera_entity(EntityRef) final { throw "unhandled"; }
};

class TestTaskCallbacks final : public TaskCallbacks {
public:
    static TestTaskCallbacks & instance() {
        static TestTaskCallbacks inst;
        return inst;
    }

    void add(const SharedPtr<EveryFrameTask> &) final { throw "unhandled"; }

    void add(const SharedPtr<BackgroundTask> &) final { throw "unhandled"; }

    void add(const Entity &) final { throw "unhandled"; }

    void add(const SharedPtr<TriangleLink> &) final { throw "unhandled"; }

    void remove(const SharedPtr<const TriangleLink> &) final { throw "unhandled"; }

    Platform & platform() final
        { return TestPlatform::instance(); }
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe("TiledMapLoader")([] {
    mark_it("stuff", [] {
        auto & content_loader = TestMapContentLoader::instance();
        auto sm = tiled_map_loading::MapLoadStateMachine::
            make_with_starting_state
                (content_loader, TestMapContentLoader::k_test_map);
        sm.update_progress(content_loader);
        (void)content_loader.waited_on_tasks.front()->in_background
            (TestTaskCallbacks::instance(),
             TestMapContentLoader::continuation_strategy());
        sm.update_progress(content_loader);
        auto res = sm.update_progress(content_loader);
        return test_that(res.is_right());
    });
});

return [] {};

} ();
