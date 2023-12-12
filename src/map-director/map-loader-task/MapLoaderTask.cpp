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

#include "MapLoaderTask.hpp"
#include "ProducablesTileset.hpp"

#include "../../Definitions.hpp"

namespace {

using MapLoadResult = tiled_map_loading::BaseState::MapLoadResult;
using Continuation = BackgroundTask::Continuation;
using TaskContinuation = MapContentLoaderComplete::TaskContinuation;
using FillerFactoryMap = ProducablesTileset::FillerFactoryMap;
using Result = MapLoaderTask::Result;

} // end of <anonymous> namespace

MapLoaderTask::MapLoaderTask
    (const char * map_filename, PlatformAssetsStrategy & assets_strategy)
{
    using namespace tiled_map_loading;

    m_content_loader.assign_assets_strategy(assets_strategy);
    m_map_loader = MapLoadStateMachine::make_with_starting_state
        (m_content_loader, map_filename);
}

Continuation & MapLoaderTask::in_background
    (Callbacks & callbacks, ContinuationStrategy & strat)
{
    m_content_loader.assign_assets_strategy(callbacks.platform());
    m_content_loader.assign_continuation_strategy(strat);
    m_map_loader.update_progress(m_content_loader).fold<int>(0).
        map([this] (MapLoadingSuccess && res) {
            m_map_result.map_region = std::move(res.loaded_region);
            m_map_result.map_objects = std::move(res.object_collection);
            return 0;
        }).
        map_left([] (MapLoadingError &&) {
            throw RuntimeError{"Failed to load map"};
            return 0;
        }).
        value();
    return m_content_loader.task_continuation();
}

Result MapLoaderTask::retrieve() {
    if (!m_map_result.map_region) {
        throw RuntimeError{"No loaded region to retrieve"};
    }
    return std::move(m_map_result);
}

// ----------------------------------------------------------------------------

MapContentLoaderComplete::MapContentLoaderComplete(Platform & platform):
    m_platform(&platform) {}

const FillerFactoryMap & MapContentLoaderComplete::map_fillers() const {
#   ifdef MACRO_DEBUG
    assert(m_filler_map);
#   endif
    return *m_filler_map;
}

FutureStringPtr MapContentLoaderComplete::promise_file_contents
    (const char * filename)
{ return m_platform->promise_file_contents(filename); }

SharedPtr<Texture> MapContentLoaderComplete::make_texture() const
    { return m_platform->make_texture(); }

void MapContentLoaderComplete::wait_on
    (const SharedPtr<BackgroundTask> & task)
    { m_continuation = &m_strategy->continue_().wait_on(task); }

void MapContentLoaderComplete::assign_continuation_strategy
    (ContinuationStrategy & strategy)
{
    m_strategy = &strategy;
    m_continuation = &strategy.finish_task();
}

TaskContinuation & MapContentLoaderComplete::task_continuation() const {
    if (m_continuation) return *m_continuation;
    throw RuntimeError{"Strategy was not set"};
}
