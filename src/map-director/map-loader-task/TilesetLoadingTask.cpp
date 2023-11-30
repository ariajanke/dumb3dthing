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

#include <tinyxml2.h>

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
    if (m_loaded_tile_set) {
        return strategy.finish_task();
    } else if (m_unloaded.tile_set) {
        // somehow inject my own fillers...
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
    Either<MapLoadingError, TilesetLoadingTask::UnloadedTileSet>
    TilesetLoadingTask::get_unloaded
    (FutureStringPtr & tile_set_content)
{
    using FutureLost = Future<std::string>::Lost;
    return tile_set_content->retrieve().require().
        map_left([] (FutureLost &&) {
            return MapLoadingError{map_loading_messages::k_tile_map_file_contents_not_retrieved};
        }).
        chain(DocumentOwningNode::load_root).
        chain([]
            (DocumentOwningNode && node) ->
                Either<MapLoadingError, UnloadedTileSet>
        {
            auto ts = TilesetBase::make(node.element());
            if (!ts) return MapLoadingError{map_loading_messages::k_tile_map_file_contents_not_retrieved};
            return UnloadedTileSet{std::move(ts), std::move(node)};
        });
}

// ----------------------------------------------------------------------------

/* static */ Either<MapLoadingError, DocumentOwningNode>
    DocumentOwningNode::load_root(std::string && file_contents)
{
    using namespace map_loading_messages;
    struct OwnerImpl final : public Owner {
        TiXmlDocument document;
    };
    // great, guaranteed dynamic allocation
    // and file_contents is consumed
    auto owner = make_shared<OwnerImpl>();
    auto & document = owner->document;
    if (document.Parse(file_contents.c_str()) != tinyxml2::XML_SUCCESS) {
        return MapLoadingError{k_tile_map_file_contents_not_retrieved};
    }
    return DocumentOwningNode{owner, *document.RootElement()};
}

/* static */ OptionalEither<MapLoadingError, DocumentOwningNode>
    DocumentOwningNode::optionally_load_root(std::string && file_contents)
{
    using Rt = OptionalEither<MapLoadingError, DocumentOwningNode>;
    auto ei = load_root(std::move(file_contents));
    if (ei.is_left()) {
        return Rt{ei.left()};
    }
    return Rt{ei.right()};
}

DocumentOwningNode DocumentOwningNode::make_with_same_owner
    (const TiXmlElement & same_document_element) const
{ return DocumentOwningNode{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningNode::element() const
    { return *m_element; }
