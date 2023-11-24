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

#include "../../Definitions.hpp"
#include "../../TasksController.hpp"

namespace {

using namespace cul::exceptions_abbr;
using MapLoadResult = tiled_map_loading::BaseState::MapLoadResult;
using Continuation = BackgroundTask::Continuation;

} // end of <anonymous> namespace

MapLoaderTask::MapLoaderTask(const char * map_filename, Platform & platform) {
    using namespace tiled_map_loading;

    m_content_loader.assign_platform(platform);
    m_map_loader = MapLoadStateMachine::make_with_starting_state
        (m_content_loader, map_filename);
}

Continuation & MapLoaderTask::in_background
    (Callbacks & callbacks, ContinuationStrategy & strat)
{
    m_content_loader.assign_platform(callbacks.platform());
    m_content_loader.assign_continuation_strategy(strat);
    (void)m_map_loader.update_progress(m_content_loader).fold<int>(0).
        map([this] (MapLoadingSuccess && res) {
            m_loaded_region = std::move(res.loaded_region);
            return 0;
        }).
        map_left([] (MapLoadingError &&) {
            return 0;
        }).
        value();
    return m_content_loader.continuation();
}

UniquePtr<MapRegion> MapLoaderTask::retrieve() {
    if (!m_loaded_region) {
        throw std::runtime_error{"No loaded region to retrieve"};
    }
    return std::move(m_loaded_region);
}

// ----------------------------------------------------------------------------

MapContentLoaderComplete::MapContentLoaderComplete(Platform & platform):
    m_platform(&platform) {}

FutureStringPtr MapContentLoaderComplete::promise_file_contents
    (const char * filename)
    { return m_platform->promise_file_contents(filename); }

SharedPtr<Texture> MapContentLoaderComplete::make_texture() const
    { return m_platform->make_texture(); }

void MapContentLoaderComplete::wait_on
    (const SharedPtr<BackgroundTask> & task)
    { m_background_task_trap.add(task); }
