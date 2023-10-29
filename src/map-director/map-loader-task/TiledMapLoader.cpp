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
#if 0
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
{ m_element->ToDocument(); return DocumentOwningNode{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningNode::element() const
    { return *m_element; }
#endif

// ----------------------------------------------------------------------------

BaseState::MapLoadResult
    FileContentsWaitState::update_progress(StateSwitcher & switcher)
{
    return m_future_contents->retrieve().
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
#if 0
// there is definitely multiple classes hiding here
class FutureTileSet final {
public:
    using FutureLost = Future<std::string>::Lost;
    using Readiness = Future<std::string>::Readiness;

    static FutureTileSet begin_loading
        (const char * filename, FileContentProvider & content_provider)
        { return FutureTileSet{content_provider.promise_file_contents(filename)}; }

    static FutureTileSet begin_loading
        (DocumentOwningNode && tileset_xml)
    {
        // NOTE will still exist following move of tileset_xml
        const auto & el = tileset_xml.element();
        return FutureTileSet
            {UnloadedTileSet{TileSetBase::make(el), std::move(tileset_xml)}};
    }

    // I like it being required better, may have an "update" method?
    // something which I also hate...
    OptionalEither<MapLoadingError, SharedPtr<TileSetBase>>
        retrieve_from(FileContentProvider & content_provider)
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

private:
    struct UnloadedTileSet final {
        UnloadedTileSet() {};

        UnloadedTileSet
            (SharedPtr<TileSetBase> && tile_set_,
             DocumentOwningNode && xml_content_):
            tile_set(std::move(tile_set_)),
            xml_content(std::move(xml_content_)) {}

        SharedPtr<TileSetBase> tile_set;
        DocumentOwningNode xml_content;
    };

    explicit FutureTileSet(FutureStringPtr && content_):
        m_tile_set_content(std::move(content_)) {}

    explicit FutureTileSet(UnloadedTileSet && unloaded_ts_):
        m_unloaded(std::move(unloaded_ts_)) {}

    static Either<MapLoadingError, UnloadedTileSet> get_unloaded
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

    UnloadedTileSet m_unloaded;
    SharedPtr<TileSetBase> m_loaded_tile_set;
    FutureStringPtr m_tile_set_content;
    Optional<Readiness> m_post_load_readiness;
};

struct FutureTileSetWithStartGid final {
    FutureTileSetWithStartGid
        (FutureTileSet && future_tile_set_, int start_gid_):
        future_tile_set(std::move(future_tile_set_)),
        start_gid(start_gid_) {}

    FutureTileSet future_tile_set;
    int start_gid = -1;
};
#endif
/* static */ Optional<FutureTileSetWithStartGid> load_balskjdlaskjdlakjs
    (const DocumentOwningNode & tileset,
     FileContentProvider & content_provider,
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
        return FutureTileSetWithStartGid
            {FutureTileSet::begin_loading(source, content_provider),
             first_gid};
    } else {
        return FutureTileSetWithStartGid
            {FutureTileSet::begin_loading(DocumentOwningNode{tileset}),
             first_gid};
    }
}

#if 0
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
        platform.promise_file_contents(source)->retrieve().
            map([] (std::string && contents) {
                DocumentOwningNode::load_root(std::move(contents));
                return 0;
            });

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
    auto res = m_future_string->retrieve();
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
#endif
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
#if 0
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
#endif

/* static */ std::vector<FutureTileSetWithStartGid>
    InitialDocumentReadState::load_future_tilesets
    (const DocumentOwningNode & document_root,
     MapLoadingWarningsAdder & warnings,
     FileContentProvider & content_provider)
{
    std::vector<FutureTileSetWithStartGid> future_tilesets;
    for (auto & tileset : XmlRange{*document_root, "tileset"}) {
        auto res = load_balskjdlaskjdlakjs
            (document_root.make_with_same_owner(tileset),
             content_provider, warnings);
        if (!res) continue;
        future_tilesets.emplace_back(std::move(*res));
    }
    return future_tilesets;
}

MapLoadResult InitialDocumentReadState::update_progress
    (StateSwitcher & switcher)
{
    auto layers = load_layers(*m_document_root, warnings_adder());
    auto future_tilesets = load_future_tilesets
        (m_document_root, warnings_adder(), content_provider());
    // WE DON'T WAIT!! (not here anyhow)
    switcher.set_next_state<TileSetLoadState>
        (std::move(m_document_root),
         std::move(layers),
         std::move(future_tilesets));
    return MapLoadResult{};
}

// ----------------------------------------------------------------------------

TileSetLoadState::TileSetLoadState
    (DocumentOwningNode && document_root_,
     std::vector<Grid<int>> && layers_,
     std::vector<FutureTileSetWithStartGid> && future_tilesets_):
    m_document_root(std::move(document_root_)),
    m_layers(std::move(layers_)),
    m_future_tilesets(std::move(future_tilesets_)) {}

MapLoadResult TileSetLoadState::update_progress
    (StateSwitcher & state_switcher)
{
    for (auto & pair : m_future_tilesets) {
        // pair.future_tile_set.
    }
}

#if 0
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
#endif
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
    (FileContentProvider & provider, const char * filename)
    { initialize_starting_state(provider, filename); }

void MapLoadStateMachine::initialize_starting_state
    (FileContentProvider & provider, const char * filename)
{
    auto file_contents_promise = provider.promise_file_contents(filename);
    m_state_driver.
        set_current_state<FileContentsWaitState>
        (std::move(file_contents_promise), provider);
    m_content_provider = &provider;
}

MapLoadResult MapLoadStateMachine::update_progress() {
    auto switcher = m_state_driver.state_switcher();
    auto result = m_state_driver.
        on_advanceable<&BaseState::copy_platform_and_warnings>().
        advance()->
        update_progress(switcher);
    if (result.is_empty() &&
        (m_state_driver.is_advanceable() ||
         !m_content_provider->delay_required()))
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
