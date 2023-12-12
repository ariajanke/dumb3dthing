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

#include "TiledMapLoader.hpp"

#include "../map-loader-task.hpp"

class MapContentLoaderComplete final : public MapContentLoader {
public:
    using TaskContinuation = BackgroundTask::Continuation;
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;

    MapContentLoaderComplete() {}

    explicit MapContentLoaderComplete(Platform &);

    const FillerFactoryMap & map_fillers() const final;

    bool delay_required() const final
        { return &task_continuation() == &m_strategy->continue_(); }

    FutureStringPtr promise_file_contents(const char * filename) final;

    void add_warning(MapLoadingWarningEnum) final {}

    SharedPtr<Texture> make_texture() const final;

    void wait_on(const SharedPtr<BackgroundTask> &) final;

    void assign_assets_strategy(PlatformAssetsStrategy & platform)
        { m_platform = &platform; }

    void assign_continuation_strategy(ContinuationStrategy &);

    void assign_filler_map(const FillerFactoryMap & filler_map)
        { m_filler_map = &filler_map; }

    SharedPtr<RenderModel> make_render_model() const final
        { return m_platform->make_render_model(); }

    TaskContinuation & task_continuation() const final;

private:
    PlatformAssetsStrategy * m_platform = nullptr;
    ContinuationStrategy * m_strategy = nullptr;
    TaskContinuation * m_continuation = nullptr;
    const FillerFactoryMap * m_filler_map = &MapContentLoader::builtin_fillers();
};

// ----------------------------------------------------------------------------

class MapLoaderTask final : public MapLoaderTask_ {
public:
    MapLoaderTask(const char * map_filename, PlatformAssetsStrategy &);

    Continuation & in_background
        (Callbacks &, ContinuationStrategy &) final;

    Result retrieve() final;

private:
    Result m_map_result;
    tiled_map_loading::MapLoadStateMachine m_map_loader;
    MapContentLoaderComplete m_content_loader;
};
