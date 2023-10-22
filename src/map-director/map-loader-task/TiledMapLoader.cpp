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

#include "TiledMapLoader.hpp"
#include "TileSet.hpp"

#include <ariajanke/cul/Either.hpp>

#include <tinyxml2.h>

#include <cstring>

namespace {

using namespace cul::exceptions_abbr;

using MapLoadResult = tiled_map_loading::BaseState::MapLoadResult;

Either<MapLoadingWarningEnum, Grid<int>> load_layer_(const TiXmlElement &);

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

namespace tiled_map_loading {

void BaseState::copy_platform_and_warnings(const BaseState & rhs) {
    m_platform = rhs.m_platform;
    m_unfinished_warnings = rhs.m_unfinished_warnings;
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

DocumentOwningNode DocumentOwningNode::make_with_same_owner
    (const TiXmlElement & same_document_element) const
{ m_element->ToDocument(); return DocumentOwningNode{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningNode::element() const
    { return *m_element; }


// ----------------------------------------------------------------------------

BaseState::MapLoadResult
    FileContentsWaitState::update_progress(StateSwitcher & switcher)
{
    return (*m_future_contents)().
    map_left([] (Future<std::string>::Lost) {
        return MapLoadingError
            {map_loading_messages::k_tile_map_file_contents_not_retrieved};
    }).
    chain([&] (std::string && contents) {
        using namespace map_loading_messages;
        return DocumentOwningNode::load_root(std::move(contents)).
            fold<MapLoadResult>().
            map([&] (DocumentOwningNode && root) {
                switcher.set_next_state<InitialDocumentReadState>(std::move(root));
                return MapLoadResult{};
            }).
            map_left([] (MapLoadingError error) {
                return MapLoadResult{std::move(error)};
            }).
            value();
    });
}

// ----------------------------------------------------------------------------

/* static */ Optional<UnfinishedTileSetContent> UnfinishedTileSetContent::load
    (const DocumentOwningNode & tileset,
     Platform & platform,
     MapLoadingWarningsAdder & warnings)
{
    using namespace cul::either;
    using namespace map_loading_messages;
    constexpr const int k_no_first_gid = -1;

    auto first_gid = tileset->IntAttribute("firstgid", k_no_first_gid);
    if (first_gid == k_no_first_gid) {
        warnings.add(k_invalid_tile_data);
        return {};
    }

    if (const auto * source = tileset->Attribute("source")) {
        return UnfinishedTileSetContent
            {first_gid, platform.promise_file_contents(source)};
    } else {
        return UnfinishedTileSetContent{first_gid, tileset};
    }
}

Either<UnfinishedTileSetContent::Lost, Optional<TileSetContent>>
    UnfinishedTileSetContent::update()
{
    if (m_tileset_element) {
        return cul::either::right<Lost>().with(finish());
    }
    auto res = (*m_future_string)();
    if (res.is_empty()) { return Optional<TileSetContent>{}; }
    m_future_string = nullptr;
    return res.
        require().
        chain([this] (std::string && contents) {
            return DocumentOwningNode::load_root(std::move(contents)).
                map_left([] (MapLoadingError) {
                    // error lost :c
                    return Lost{};
                }).
                map([this] (DocumentOwningNode && node)
                    { return finish(std::move(node)); });
            });
}

bool UnfinishedTileSetContent::is_finished() const
    { return !m_future_string && !m_tileset_element; }

/* private */ Optional<TileSetContent> UnfinishedTileSetContent::finish()
    { return finish(std::move(m_tileset_element)); }

/* private */ Optional<TileSetContent> UnfinishedTileSetContent::finish
    (DocumentOwningNode && node)
{
    if (!node) {
        using namespace cul::exceptions_abbr;
        throw RtError{"UnfinishedTileSetContent::finish: owning node must "
                      "refer to an element"};
    }
    return TileSetContent{m_first_gid, std::move(node)};
}

/* private */ UnfinishedTileSetContent::UnfinishedTileSetContent
    (int first_gid, FutureStringPtr && future_):
    m_first_gid(first_gid),
    m_future_string(std::move(future_)) {}

/* private */ UnfinishedTileSetContent::UnfinishedTileSetContent
    (int first_gid, const DocumentOwningNode & tilset_element):
    m_first_gid(first_gid),
    m_tileset_element(tilset_element) {}

// ----------------------------------------------------------------------------

/* static */ std::vector<Grid<int>>
    InitialDocumentReadState::load_layers
    (const TiXmlElement & document_root, MapLoadingWarningsAdder & warnings)
{
    std::vector<Grid<int>> layers;
    for (auto & layer_el : XmlRange{document_root, "layer"}) {
        auto ei = load_layer_(layer_el);
        if (ei.is_left()) {
            warnings.add(ei.left());
        } else if (ei.is_right()) {
            layers.emplace_back(ei.right());
        }
    }
    return layers;
}

/* static */ std::vector<UnfinishedTileSetContent>
    InitialDocumentReadState::load_unfinished_tilesets
    (const DocumentOwningNode & document_root,
     MapLoadingWarningsAdder & warnings,
     Platform & platform)
{
    std::vector<UnfinishedTileSetContent> unfinished_tilesets;
    for (auto & tileset : XmlRange{*document_root, "tileset"}) {
        auto unfinished_content = UnfinishedTileSetContent::load
            (document_root.make_with_same_owner(tileset), platform, warnings);
        if (unfinished_content) {
            unfinished_tilesets.emplace_back(*std::move(unfinished_content));
        }
    }
    return unfinished_tilesets;
}

MapLoadResult InitialDocumentReadState::update_progress
    (StateSwitcher & switcher)
{
    auto layers = load_layers(*m_document_root, warnings_adder());
    auto unfinished_tilesets = load_unfinished_tilesets
        (m_document_root, warnings_adder(), platform());
    switcher.set_next_state<TileSetWaitState>
        (std::move(m_document_root),
         std::move(layers),
         std::move(unfinished_tilesets));
    return MapLoadResult{};
}

// ----------------------------------------------------------------------------

/* static */ TileSetWaitState::UpdatedContainers
    TileSetWaitState::UpdatedContainers::update
    (std::vector<UnfinishedTileSetContent> && unfinished_container,
     std::vector<TileSetContent> && finished_container,
     MapLoadingWarningsAdder & warnings)
{
    // this is two functions :S
    using Lost = UnfinishedTileSetContent::Lost;
    for (auto & unfinished : unfinished_container) {
        (void)unfinished.update().fold<int>().
            map([&finished_container] (Optional<TileSetContent> && finished) {
                if (finished) {
                    finished_container.emplace_back(*std::move(finished));
                }
                return 0;
            }).
            map_left([&warnings] (Lost) {
                // issue a warning at some point
                (void)warnings;
                return 0;
            }).
            value();
    }

    unfinished_container.erase
        (std::remove_if(unfinished_container.begin(),
                        unfinished_container.end  (),
                        UnfinishedTileSetContent::finishable),
         unfinished_container.end());
    return UpdatedContainers{std::move(unfinished_container),
                             std::move(finished_container  )};
}

TileSetWaitState::UpdatedContainers::UpdatedContainers
    (std::vector<UnfinishedTileSetContent> && unfinished,
     std::vector<TileSetContent> && finished):
    m_unfinished(std::move(unfinished)),
    m_finished(std::move(finished)) {}

std::vector<UnfinishedTileSetContent>
    TileSetWaitState::UpdatedContainers::move_out_unfinished()
    { return std::move(m_unfinished); }

std::vector<TileSetContent>
    TileSetWaitState::UpdatedContainers::move_out_finished()
    { return std::move(m_finished); }

TileSetWaitState::TileSetWaitState
    (DocumentOwningNode && document_root,
     std::vector<Grid<int>> && layers,
     std::vector<UnfinishedTileSetContent> && unfinished_tilesets):
    m_document_root(std::move(document_root)),
    m_layers(std::move(layers)),
    m_unfinished_contents(std::move(unfinished_tilesets)) {}

MapLoadResult TileSetWaitState::update_progress(StateSwitcher & switcher) {
    auto updated = UpdatedContainers::update
        (std::move(m_unfinished_contents),
         std::move(m_finished_contents  ),
         warnings_adder()                );
    m_unfinished_contents = updated.move_out_unfinished();
    m_finished_contents = updated.move_out_finished();
    if (m_unfinished_contents.empty()) {
        switcher.set_next_state<TiledMapStrategyState>
            (std::move(m_document_root),
             std::move(m_layers),
             std::move(m_finished_contents));
    }
    return MapLoadResult{};
}

// ----------------------------------------------------------------------------

TiledMapStrategyState::TiledMapStrategyState
    (DocumentOwningNode && document_root,
     std::vector<Grid<int>> && layers,
     std::vector<TileSetContent> && finished_tilesets):
    m_document_root(std::move(document_root)),
    m_layers(std::move(layers)),
    m_finished_contents(std::move(finished_tilesets)) {}

MapLoadResult TiledMapStrategyState::update_progress
    (StateSwitcher & switcher)
{
    // for now just pass on the producable ready
    switcher.set_next_state<ProducableLoadState>
        (std::move(m_document_root),
         std::move(m_layers),
         std::move(m_finished_contents));
    return MapLoadResult{};
}

// ----------------------------------------------------------------------------

using TileSetAndStartGid = TileMapIdToSetMapping_New::TileSetAndStartGid;

/* static */ TileSetAndStartGid
    ProducableLoadState::contents_to_producables_with_start_gid
    (TileSetContent && contents,
     Platform & platform)
{
    return TileSetAndStartGid
        {TileSetBase::make_and_load_tileset(platform, contents.as_element()),
         contents.first_gid()};
}

/* static */ std::vector<TileSetAndStartGid>
    ProducableLoadState::convert_to_tileset_and_start_gids
    (std::vector<TileSetContent> && tileset_contents,
     Platform & platform)
{
    std::vector<TileSetAndStartGid> tilesets_and_start_gids;
    tilesets_and_start_gids.reserve(tileset_contents.size());
    for (auto & contents : tileset_contents) {
        tilesets_and_start_gids.emplace_back
            (contents_to_producables_with_start_gid
                (std::move(contents), platform));
    }
    return tilesets_and_start_gids;
}

ProducableLoadState::ProducableLoadState
    (DocumentOwningNode && document_root,
     std::vector<Grid<int>> && layers,
     std::vector<TileSetContent> && finished_tilesets):
    m_document_root(std::move(document_root)),
    m_layers(std::move(layers)),
    m_finished_contents(std::move(finished_tilesets)) {}

MapLoadResult ProducableLoadState::update_progress
    (StateSwitcher & switcher)
{
    class TileSetMapElementVisitorImpl final : public TileSetMapElementVisitor {
    public:
        void add(StackableProducableTileGrid && stackable) final {
            m_stackable_producable_grid = m_stackable_producable_grid.
                stack_with(std::move(stackable));
        }

        void add(StackableSubRegionGrid &&) final
            { throw std::runtime_error{"not implemented"}; }

        ProducableTileViewGrid to_producables()
            { return m_stackable_producable_grid.to_producables(); }

    private:
        StackableProducableTileGrid m_stackable_producable_grid;
    };

    TileMapIdToSetMapping_New set_mapping
        {convert_to_tileset_and_start_gids(std::move(m_finished_contents), platform())};
    TileSetMapElementVisitorImpl impl;
    for (auto & layer : m_layers) {
        auto layer_mapping = set_mapping.make_mapping_for_layer(layer);
        for (auto & layer_wrapper : layer_mapping) {
            auto & tileset = *TileSetMappingLayer::tileset_of
                (layer_wrapper.as_view());
            // ends up consuming the tileset :c
            tileset.add_map_elements(impl, layer_wrapper);
        }
    }
    MapLoadingSuccess success;
    success.loaded_region =
        make_unique<TiledMapRegion>(impl.to_producables(), map_scale());
    switcher.set_next_state<ExpiredState>();
    return success;
}

/* private */ ScaleComputation ProducableLoadState::map_scale() const {
    // there maybe more properties in the future, but for now there's only one
    auto props = m_document_root->FirstChildElement("properties");
    for (auto & el : XmlRange{props, "property"}) {
        if (::strcmp(el.Attribute("name"), "scale") == 0) {
            auto scale = ScaleComputation::parse(el.Attribute("value"));
            if (scale)
                { return *scale; }
        }
    }
    return ScaleComputation{};
}

// ----------------------------------------------------------------------------

MapLoadStateMachine::MapLoadStateMachine
    (Platform & platform, const char * filename)
    { initialize_starting_state(platform, filename); }

void MapLoadStateMachine::initialize_starting_state
    (Platform & platform, const char * filename)
{
    auto file_contents_promise = platform.promise_file_contents(filename);
    m_state_driver.
        set_current_state<FileContentsWaitState>
        (std::move(file_contents_promise), platform);
}

MapLoadResult MapLoadStateMachine::update_progress() {
    auto switcher = m_state_driver.state_switcher();
    auto result = m_state_driver.
        on_advanceable<&BaseState::copy_platform_and_warnings>().
        advance()->
        update_progress(switcher);
    if (result.is_empty() && m_state_driver.is_advanceable())
        { return update_progress(); }
    return result;
}

} // end of tiled_map_loading namespace

// ----------------------------------------------------------------------------

namespace {

static const auto k_whitespace_trimmer = make_trim_whitespace<const char *>();

Either<MapLoadingWarningEnum, Grid<int>> load_layer_(const TiXmlElement & layer_el) {
    using namespace map_loading_messages;
    using namespace cul::either;
    Grid<int> layer;
    layer.set_size
        (layer_el.IntAttribute("width"), layer_el.IntAttribute("height"), 0);

    auto * data = layer_el.FirstChildElement("data");
    if (!data) {
        return k_tile_layer_has_no_data_element;
    } else if (::strcmp(data->Attribute( "encoding" ), "csv")) {
        return k_non_csv_tile_data;
    }

    auto data_text = data->GetText();
    if (!data_text)
        { return layer; }

    Vector2I r;
    for (auto value_str : split_range(data_text, data_text + ::strlen(data_text),
                                      is_comma, k_whitespace_trimmer))
    {
        int tile_id = 0;
        bool entry_is_numeric = cul::string_to_number
            (value_str.begin(), value_str.end(), tile_id);
        if (!entry_is_numeric) {
            return left(k_invalid_tile_data).with<Grid<int>>();
        }
        // should warn if not a number
        layer(r) = tile_id;
        r = layer.next(r);
    }
    return layer;
}

} // end of <anonymous> namespace
