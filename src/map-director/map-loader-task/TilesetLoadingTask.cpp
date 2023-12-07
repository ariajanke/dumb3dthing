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

#include "TilesetLoadingTask.hpp"
#include "MapLoaderTask.hpp"

#include "TiledMapLoader.hpp"
#include "TilesetBase.hpp"

#include <ariajanke/cul/Either.hpp>


namespace {

using Continuation = BackgroundTask::Continuation;

} // end of <anonymous> namespace

/* static */ TilesetLoadingTask TilesetLoadingTask::begin_loading
    (const char * filename, MapContentLoader & content_provider)
{
    return TilesetLoadingTask
        {content_provider.promise_file_contents(filename),
         content_provider.map_fillers()};
}

/* static */ TilesetLoadingTask TilesetLoadingTask::begin_loading
    (DocumentOwningNode && tileset_xml, MapContentLoader & content_provider)
{
    const auto & el = tileset_xml.element();
    return TilesetLoadingTask
        {UnloadedTileSet{TilesetBase::make(el), std::move(tileset_xml)},
         content_provider.map_fillers()};
}

Continuation & TilesetLoadingTask::in_background
    (Callbacks & callbacks, ContinuationStrategy & strategy)
{
    if (m_loaded_tile_set || m_loading_error) {
        return strategy.finish_task();
    } else if (m_unloaded.tile_set) {
        MapContentLoaderComplete content_loader;
        content_loader.assign_assets_strategy(callbacks.platform());
        content_loader.assign_continuation_strategy(strategy);
        content_loader.assign_filler_map(*m_filler_factory_map);
        auto & res = m_unloaded.tile_set->load
            (m_unloaded.xml_content.element(), content_loader);
        m_loaded_tile_set = std::move(m_unloaded.tile_set);
        m_unloaded = UnloadedTileSet{};
        return res;
    } else {
        auto ei = get_unloaded(m_tile_set_content).
            map_left([](MapLoadingError && error)
                     { return Optional<MapLoadingError>{std::move(error)}; });
        m_loading_error = ei.left_or(Optional<MapLoadingError>{});
        m_unloaded = ei.right_or(UnloadedTileSet{});
    }
    return strategy.continue_();
}

OptionalEither<MapLoadingError, SharedPtr<TilesetBase>>
    TilesetLoadingTask::retrieve()
{
    if (m_loading_error) return *m_loading_error;
    if (m_loaded_tile_set) return m_loaded_tile_set;
    return {};
}

/* private static */
    OptionalEither<MapLoadingError, TilesetLoadingTask::UnloadedTileSet>
    TilesetLoadingTask::get_unloaded
    (FutureStringPtr & tile_set_content)
{
    static constexpr const auto k_not_retrieved =
        map_loading_messages::k_tile_map_file_contents_not_retrieved;
    using FutureLost = Future<std::string>::Lost;
    return tile_set_content->retrieve().
        map_left([] (FutureLost &&) {
            return MapLoadingError{k_not_retrieved};
        }).
        chain(optionally_load_root).
        chain([]
            (DocumentOwningNode && node) ->
                OptionalEither<MapLoadingError, UnloadedTileSet>
        {
            auto ts = TilesetBase::make(node.element());
            if (!ts) return MapLoadingError{k_not_retrieved};
            return UnloadedTileSet{std::move(ts), std::move(node)};
        });
}

/* private static */ OptionalEither<MapLoadingError, DocumentOwningNode>
    TilesetLoadingTask::optionally_load_root(std::string && file_contents)
{
    auto ei = MapLoadingError::failed_load_as_error
        (DocumentOwningNode::load_root(std::move(file_contents)));
    if (ei.is_left()) return ei.left();
    return ei.right();
}
