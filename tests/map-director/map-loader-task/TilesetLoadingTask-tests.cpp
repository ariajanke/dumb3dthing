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

#include "../../../src/map-director/map-loader-task/TilesetLoadingTask.hpp"
#include "TestMapContentLoader.hpp"
#include "../../test-helpers.hpp"
#include "../../platform.hpp"

#include <tinyxml2.h>


namespace {

using namespace cul::tree_ts;

static constexpr const auto k_test_tileset_content =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<tileset version=\"1.8\" tiledversion=\"1.8.0\" name=\"test-tileset\""
"         tilewidth=\"32\" tileheight=\"32\" tilecount=\"1\" columns=\"1\">"
 "<image source=\"tileset.png\" width=\"32\" height=\"32\"/>"
 "<tile id=\"0\" type=\"test-tile-type\">"
  "<properties>"
   "<property name=\"sample-prop\" value=\"sample-value\"/>"
  "</properties>"
 "</tile>"
 "</tileset>";

class TestMapContentLoader final : public TestMapContentLoaderCommon {
public:
    static constexpr const auto k_test_tileset = "ggg";

    static TestMapContentLoader & instance() {
        static TestMapContentLoader inst;
        return inst;
    }

    FutureStringPtr promise_file_contents(const char * fn) const final {
        if (::strcmp(fn, k_test_tileset))
            { throw "unhandled"; }
        class Impl final : public Future<std::string> {
            OptionalEither<Lost, std::string> retrieve() {
                if (!TestMapContentLoader::instance().file_contents_available())
                    return {};
                return std::string{k_test_tileset_content};
            }
        };
        return make_shared<Impl>();
    }

    void make_file_contents_available() { m_file_contents_available = true; }

    bool file_contents_available() const { return m_file_contents_available; }

private:
    bool m_file_contents_available = false;
};

class TestTaskCallbacks final : public TaskCallbacks {
public:
    static TestTaskCallbacks & instance() {
        static TestTaskCallbacks inst;
        return inst;
    }

    void add(const SharedPtr<EveryFrameTask> &) final {}

    void add(const SharedPtr<BackgroundTask> &) final {}

    void add(const Entity &) final {}

    void add(const SharedPtr<TriangleLink> &) final {}

    void remove(const SharedPtr<const TriangleLink> &) final {}

    Platform & platform() final
        { return TestPlatform::null_instance(); }
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe("TilesetLoadingTask")([] {
    TestMapContentLoader::instance() = TestMapContentLoader{};
    auto task = TilesetLoadingTask::begin_loading
        (TestMapContentLoader::k_test_tileset, TestMapContentLoader::instance());
    mark_it("Does not finish until content is ready", [&] {
        auto & callbacks = TestTaskCallbacks::instance();
        auto & strat = TestMapContentLoader::instance().coninuation_strategy;
        for (int i = 0; i != 3; ++i) {
            auto & continuation = task.in_background(callbacks, strat);
            if (&continuation == &BackgroundTask::Continuation::task_completion()) {
                return test_that(false);
            }
        }
        TestMapContentLoader::instance().make_file_contents_available();
        auto * cont = &task.in_background(callbacks, strat);
        cont = &task.in_background(callbacks, strat);
        return test_that(cont == &BackgroundTask::Continuation::task_completion());
    });
});

return [] {};

} ();
