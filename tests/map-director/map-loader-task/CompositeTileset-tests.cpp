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

#include "../../../src/map-director/map-loader-task/CompositeTileset.hpp"
#include "../../../src/TasksController.hpp"
#include "../../../src/map-director/map-loader-task/TiledMapLoader.hpp"

#include "../../test-helpers.hpp"

#include <tinyxml2.h>

// obscenely complicated setup :c
namespace {

using namespace cul::tree_ts;

using TaskStrategy = RunableBackgroundTasks::TaskStrategy;
using NewTaskEntry = ReturnToTasksCollection::NewTaskEntry;
using ContinuationStrategy = BackgroundTask::ContinuationStrategy;
using Continuation = BackgroundTask::Continuation;
using FillerFactoryMap = TilesetBase::FillerFactoryMap;

constexpr const auto k_test_tileset_contents =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<tileset firstgid=\"1\" name=\"comps-demo-map-parts\" tilewidth=\"32\" "
             "tileheight=\"32\" tilecount=\"4\" columns=\"2\">\n"
      "<properties>\n"
       "<property name=\"filename\" value=\"comp-map-parts.tmx\"/>\n"
       "<property name=\"type\" value=\"composite-map-tileset\"/>\n"
      "</properties>\n"
      "<image source=\"comps-demo-map-parts.png\" width=\"64\" height=\"64\"/>\n"
      "<tile id=\"1\">\n"
       "<properties>\n"
        "<property name=\"tiled-map-filename\" value=\"comp-map-parts.tmx\"/>\n"
       "</properties>\n"
      "</tile>\n"
     "</tileset>\n";

constexpr const auto k_test_map_parts_contents =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<map version=\"1.8\" tiledversion=\"1.8.0\" orientation=\"orthogonal\" "
         "renderorder=\"right-down\" width=\"10\" height=\"10\" "
         "tilewidth=\"32\" tileheight=\"32\" infinite=\"0\" nextlayerid=\"2\" "
         "nextobjectid=\"1\">"
     "<tileset firstgid=\"1\" source=\"test-tileset.tsx\"/>"
     "<layer id=\"1\" name=\"Tile Layer 1\" width=\"1\" height=\"1\">"
      "<data encoding=\"csv\">1</data>"
     "</layer>"
    "</map>";

class TestPlatform final : public Platform {
public:
    static TestPlatform & platform() {
        static TestPlatform inst;
        return inst;
    }

    void render_scene(const Scene &) final { throw ""; }

    Entity make_renderable_entity() const final { throw ""; }

    SharedPtr<Texture> make_texture() const final { throw ""; }

    SharedPtr<RenderModel> make_render_model() const final { throw ""; }

    void set_camera_entity(EntityRef) final { throw ""; }

    FutureStringPtr promise_file_contents(const char * fn) final {
        class Impl final : public Future<std::string> {
            OptionalEither<Lost, std::string> retrieve() final {
                return std::string{k_test_map_parts_contents};
            }
        };
        if (::strcmp(fn, "comp-map-parts.tmx") == 0) {
            return make_shared<Impl>();
        }
        throw "";
    }
};

class TestCallbacks final : public TaskCallbacks {
public:
    static TestCallbacks & instance() {
        static TestCallbacks inst;
        return inst;
    }

    void add(const SharedPtr<EveryFrameTask> &) final {}

    void add(const SharedPtr<BackgroundTask> &) final { throw ""; }

    void add(const Entity &) final {}

    void add(const SharedPtr<TriangleLink> &) final {}

    void remove(const SharedPtr<const TriangleLink> &) final {}

    Platform & platform() final { return TestPlatform::platform(); }
};

class TestMapContentLoader final : public MapContentLoader {
public:
    using FillerFactoryMap = TilesetBase::FillerFactoryMap;
    using TaskContinuation = BackgroundTask::Continuation;

    static TestMapContentLoader & instance() {
        static TestMapContentLoader inst;
        return inst;
    }

    const FillerFactoryMap & map_fillers() const final
        { throw ""; }

    SharedPtr<Texture> make_texture() const final { throw ""; }

    SharedPtr<RenderModel> make_render_model() const final { throw ""; }

    FutureStringPtr promise_file_contents(const char * fn) final
        { return TestPlatform::platform().promise_file_contents(fn); }

    bool delay_required() const final { throw ""; }

    void add_warning(MapLoadingWarningEnum) final {
        throw "";
    }

    void wait_on(const SharedPtr<BackgroundTask> & task) final
        { (void)continuation.wait_on(task); }

    TaskContinuation & task_continuation() const final {
        return continuation.task_completion();
    }

    TaskContinuationComplete continuation;
    std::vector<SharedPtr<BackgroundTask>> waited_on_tasks;
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe("CompositeTileset")([] {
    CompositeTileset tileset;
    TiXmlDocument doc;
    doc.Parse(k_test_tileset_contents);
    auto & continuation = TestMapContentLoader::instance().continuation;
    TaskStrategy strategy{continuation};
    (void)tileset.load(*doc.RootElement(), TestMapContentLoader::instance());
    std::vector<NewTaskEntry> new_tasks;
    ReturnToTasksCollection col;
    mark_it("waits on a map loading task", [&] {
        return test_that(continuation.has_waited_on_tasks());
    });
});

return [] {};

} ();
