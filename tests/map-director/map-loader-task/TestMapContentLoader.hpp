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

#pragma once

#include "../../../src/map-director/map-loader-task/TilesetBase.hpp"
#include "../../../src/map-director/ProducableGroupFiller.hpp"

#include <cstring>

class TestProducableTile final : public ProducableTile {
public:
    TestProducableTile() {}

    TestProducableTile(Vector2I on_map_, Vector2I on_tileset_):
        on_map(on_map_), on_tileset(on_tileset_) {}

    void operator () (ProducableTileCallbacks &) const final {}

    Vector2I on_map, on_tileset;
};

class TestProducableGroupCreation final :
    public ProducableGroupFiller::ProducableGroupCreation
{
public:
    static TestProducableGroupCreation & instance() {
        static TestProducableGroupCreation inst;
        return inst;
    }

    void reserve
        (std::size_t number_of_members, const Size2I & grid_size) final
    {
        m_test_tiles.reserve(number_of_members);
        m_grid_size = grid_size;
    }

    ProducableTile & add_member(const TileLocation & loc) {
        assert(m_test_tiles.capacity() > m_test_tiles.size());
        m_test_tiles.emplace_back(loc.on_map, loc.on_tileset);
        return m_test_tiles.back();
    }

    SharedPtr<ProducableGroupOwner> finish() {
        class Impl final : public ProducableGroupOwner {
        public:
            std::vector<TestProducableTile> made_tiles;
        };
        auto impl = make_shared<Impl>();
        impl->made_tiles = std::move(m_test_tiles);
        m_test_tiles = impl->made_tiles;
        return impl;
    }

    Size2I grid_size() const { return m_grid_size; }

    const std::vector<TestProducableTile> & created_tiles() const
        { return m_test_tiles; }

private:
    Size2I m_grid_size;
    std::vector<TestProducableTile> m_test_tiles;
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

    void make_group(CallbackWithCreator & creation) const final
        { creation(TestProducableGroupCreation::instance()); }
};

class TestMapContentLoaderCommon : public MapContentLoader {
public:
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;

    TestMapContentLoaderCommon():
        coninuation_strategy(*this) {}

    SharedPtr<Texture> make_texture() const final
        { return Platform::null_callbacks().make_texture(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return Platform::null_callbacks().make_render_model(); }

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

    using Continuation = BackgroundTask::Continuation;
    class ContinuationImpl final : public Continuation {
    public:
        explicit ContinuationImpl(TestMapContentLoaderCommon & parent):
            m_parent(&parent) {}

        Continuation & wait_on(const SharedPtr<BackgroundTask> & task) {
            m_parent->wait_on(task);
            return *this;
        }

    private:
        TestMapContentLoaderCommon * m_parent = nullptr;
    };

    class ContinuationStrategyImpl final : public ContinuationStrategy {
    public:
        explicit ContinuationStrategyImpl
            (TestMapContentLoaderCommon & parent):
            impl(parent) {}

        Continuation & continue_() { return impl; }

    private:
        ContinuationImpl impl;
    };

    ContinuationStrategyImpl coninuation_strategy;
    std::vector<SharedPtr<BackgroundTask>> waited_on_tasks;
};
