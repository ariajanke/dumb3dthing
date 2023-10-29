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

#include "FutureTileSet.hpp"

#include "TiledMapLoader.hpp"

#include <tinyxml2.h>

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
{ m_element->ToDocument(); return DocumentOwningNode{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningNode::element() const
    { return *m_element; }

// ----------------------------------------------------------------------------

/* static */ FutureTileSet FutureTileSet::begin_loading
    (const char * filename, FileContentProvider & content_provider)
    { return FutureTileSet{content_provider.promise_file_contents(filename)}; }

/* static */ FutureTileSet FutureTileSet::begin_loading
    (DocumentOwningNode && tileset_xml)
{
    // NOTE will still exist following move of tileset_xml
    const auto & el = tileset_xml.element();
    return FutureTileSet
        {UnloadedTileSet{TileSetBase::make(el), std::move(tileset_xml)}};
}

OptionalEither<MapLoadingError, SharedPtr<TileSetBase>>
    FutureTileSet::retrieve_from(FileContentProvider & content_provider)
{
    if (m_loaded_tile_set) {
        return std::move(m_loaded_tile_set);
    } else if (m_unloaded.tile_set) {
        // TODO load will need to return some kind of readiness
        (void)content_provider;
#           if 0
        m_unloaded.tile_set->load(platform, m_unloaded.xml_content.element());
#           endif
        m_loaded_tile_set = std::move(m_unloaded.tile_set);
        m_unloaded = UnloadedTileSet{};
        return {};
    } else {
        auto ei = get_unloaded(m_tile_set_content);
        if (ei.is_left()) return ei.left();
        m_unloaded = ei.right();
        assert(m_unloaded.tile_set);
        return {};
    }
}

FutureTileSet::UnloadedTileSet::UnloadedTileSet
    (SharedPtr<TileSetBase> && tile_set_,
     DocumentOwningNode && xml_content_):
    tile_set(std::move(tile_set_)),
    xml_content(std::move(xml_content_)) {}

/* static */ Either<MapLoadingError, FutureTileSet::UnloadedTileSet>
    FutureTileSet::get_unloaded
    (FutureStringPtr & tile_set_content)
{
    return tile_set_content->retrieve().require().
        map_left([] (FutureLost &&) {
            return MapLoadingError{map_loading_messages::k_tile_map_file_contents_not_retrieved};
        }).
        chain(DocumentOwningNode::load_root).
        chain([]
            (DocumentOwningNode && node) ->
                Either<MapLoadingError, UnloadedTileSet>
        {
            auto ts = TileSetBase::make(node.element());
            if (!ts) return MapLoadingError{map_loading_messages::k_tile_map_file_contents_not_retrieved};
            return UnloadedTileSet{std::move(ts), std::move(node)};
        });
}
