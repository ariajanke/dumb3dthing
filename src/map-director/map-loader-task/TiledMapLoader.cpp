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
#if 0
void BaseState::copy_platform_and_warnings(const BaseState & rhs) {
    m_platform = rhs.m_platform;
    m_unfinished_warnings = rhs.m_unfinished_warnings;
}
#endif
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
    FileContentsWaitState::update_progress
    (StateSwitcher & switcher, MapContentLoader &)
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
/* static */ Optional<TileSetLoadersWithStartGid>
    load_future_tileset
    (const DocumentOwningNode & tileset,
     MapContentLoader & content_loader)
{
    using namespace cul::either;
    using namespace map_loading_messages;
    constexpr const int k_no_first_gid = -1;

    auto first_gid = tileset->IntAttribute("firstgid", k_no_first_gid);
    if (first_gid == k_no_first_gid) {
        content_loader.add_warning(k_invalid_tile_data);
        return {};
    }

    if (const auto * source = tileset->Attribute("source")) {
        return TileSetLoadersWithStartGid
            {first_gid,
             TileSetLoadingTask::begin_loading(source, content_loader)};
    } else {
        return TileSetLoadersWithStartGid
            {first_gid,
             TileSetLoadingTask::begin_loading(DocumentOwningNode{tileset})};
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
    (const TiXmlElement & document_root, MapContentLoader & content_loader)
{
    std::vector<Grid<int>> layers;
    for (auto & layer_el : XmlRange{document_root, "layer"}) {
        auto ei = load_layer_(layer_el);
        if (ei.is_left()) {
            content_loader.add_warning(ei.left());
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

/* static */ std::vector<TileSetLoadersWithStartGid>
    InitialDocumentReadState::load_future_tilesets
    (const DocumentOwningNode & document_root,
     MapContentLoader & content_loader)
{
    std::vector<TileSetLoadersWithStartGid> future_tilesets;
    for (auto & tileset : XmlRange{*document_root, "tileset"}) {
        auto res = load_future_tileset
            (document_root.make_with_same_owner(tileset), content_loader);
        if (!res) continue;
        future_tilesets.emplace_back(std::move(*res));
    }
    return future_tilesets;
}

/* static */ InitialDocumentReadState::TileSetLoadSplit
    InitialDocumentReadState::split_tileset_load
    (std::vector<TileSetLoadersWithStartGid> && tileset_load,
     MapContentLoader & content_loader)
{
    // some are available right away
    // others are not
    // ... so I end up having a type split... yuck
    TileSetLoadSplit split;
    for (auto & future : tileset_load) {
        auto res = future.other.retrieve();
        auto start_gid = future.start_gid;
        if (res.is_empty()) {
            auto task_ptr = std::make_shared<TileSetLoadingTask>
                (std::move(future.other));
            content_loader.wait_on(task_ptr);
            split.future_tilesets.emplace_back(start_gid, std::move(task_ptr));
        } else {
            (void)res.require().
                map([start_gid, &split] (SharedPtr<TileSetBase> && tileset)
            {
                split.ready_tilesets.emplace_back(start_gid, std::move(tileset));
                return 0;
            });
        }
#       if 0
        content_loader.wait_on(std::move(future.other.loader_task));
#       endif
    }
    return split;
}

MapLoadResult InitialDocumentReadState::update_progress
    (StateSwitcher & switcher, MapContentLoader & content_loader)
{
    auto layers = load_layers(*m_document_root, content_loader);
#   if 0
    auto future_tilesets = load_future_tilesets
        (m_document_root, content_loader);
#   endif
    auto load_split = split_tileset_load
        (load_future_tilesets(m_document_root, content_loader),
         content_loader                                       );
    switcher.set_next_state<TileSetLoadState>
        (std::move(m_document_root),
         std::move(layers),
         std::move(load_split.future_tilesets),
         std::move(load_split.ready_tilesets));
    return MapLoadResult{};
}

// ----------------------------------------------------------------------------
#if 0
/* static */ std::vector<FutureTileSetWithStartGid>
    TileSetLoadState::remove_done_futures
    (std::vector<FutureTileSetWithStartGid> && futures)
{
    auto rem_beg = std::remove_if(futures.begin(), futures.end(), is_done);
    futures.erase(rem_beg, futures.end());
    return std::move(futures);
}
#endif
/* static */ void TileSetLoadState::check_for_finished
    (MapContentLoader & content_provider,
     TileSetLoadersWithStartGid & future_tileset,
     StartGidWithTileSetContainer & tilesets)
{
#   if 0
    auto res = future_tileset.other.future_tileset.retrieve();
    auto start_gid = future_tileset.start_gid;
    (void)res.map([&tilesets, start_gid] (SharedPtr<TileSetBase> && tileset) {
        tilesets.emplace_back(start_gid, std::move(tileset));
        return 0;
    }).
    // TODO handle errors
    map_left([](MapLoadingError){ return 0; });
#   endif
}

/* static */ std::vector<TileSetWithStartGid>
    TileSetLoadState::finish_tilesets
    (std::vector<TileSetProviderWithStartGid> && future_tilesets,
     std::vector<TileSetWithStartGid> && ready_tilesets)
{
    ready_tilesets.reserve(ready_tilesets.size() + future_tilesets.size());
    for (auto & future : future_tilesets) {
        auto start_gid = future.start_gid;
        (void)future.other->retrieve().require().map(
            [&ready_tilesets, start_gid] (SharedPtr<TileSetBase> && tileset)
        {
            ready_tilesets.emplace_back(start_gid, std::move(tileset));
            return 0;
        });
    }
    return std::move(ready_tilesets);
}

TileSetLoadState::TileSetLoadState
    (DocumentOwningNode && document_root_,
     std::vector<Grid<int>> && layers_,
     std::vector<TileSetProviderWithStartGid> && future_tilesets_,
     std::vector<TileSetWithStartGid> && ready_tilesets_):
    m_document_root(std::move(document_root_)),
    m_layers(std::move(layers_)),
    m_future_tilesets(std::move(future_tilesets_)),
    m_ready_tilesets(std::move(ready_tilesets_)) {}

MapLoadResult TileSetLoadState::update_progress
    (StateSwitcher & state_switcher, MapContentLoader &)
{
#   if 0
    check_for_finished(content_loader);
#   endif
#   if 0
    m_future_tilesets = remove_done_futures(std::move(m_future_tilesets));
    if (m_future_tilesets.empty()) {

    }
#   endif
    auto finished_tilesets = finish_tilesets
        (std::move(m_future_tilesets), std::move(m_ready_tilesets));
#   if 0
    for (auto & future_tileset : m_future_tilesets) {

#       if 0
        check_for_finished(content_provider, future_tileset, m_tilesets);
#       endif
    }
#   endif
#   if 0
    state_switcher.set_next_state<MapElementCollectorState>
        (std::move(m_document_root),
         TileMapIdToSetMapping_New{ std::move(m_tilesets) },
         std::move(m_layers));
#   endif
    state_switcher.set_next_state<MapElementCollectorState>
        (std::move(m_document_root),
         TileMapIdToSetMapping_New{ std::move(finished_tilesets) },
         std::move(m_layers));
    return {};
}
#if 0
/* private */ void TileSetLoadState::check_for_finished
    (MapContentLoader & content_provider)
{
    for (auto & future_tileset : m_future_tilesets) {
        check_for_finished(content_provider, future_tileset, m_tilesets);
    }
}
#endif
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
#if 0
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
#endif

MapElementCollectorState::MapElementCollectorState
    (DocumentOwningNode && document_root_,
     TileMapIdToSetMapping_New && mapping_,
     std::vector<Grid<int>> && layers_):
    m_document_root(std::move(document_root_)),
    m_id_mapping_set(std::move(mapping_)),
    m_layers(std::move(layers_)) {}

class MapRegionBuilder final : public TileSetMapElementVisitor {
public:
    static MapRegionBuilder
        load_from_elements(TileMapIdToSetMapping_New && id_mapping_set,
                           std::vector<Grid<int>> && layers);

    MapRegionBuilder() {}

    void add(StackableProducableTileGrid && layer) final {
        m_stackable_producables = m_stackable_producables.
            stack_with(std::move(layer));
    }

    void add(StackableSubRegionGrid &&) final {}

    UniquePtr<TiledMapRegion> make_map_region(ScaleComputation && scale) {
        return std::make_unique<TiledMapRegion>
            (m_stackable_producables.to_producables(), std::move(scale));
    }

private:
    StackableProducableTileGrid m_stackable_producables;
};

/* static */ MapRegionBuilder
    MapRegionBuilder::load_from_elements
    (TileMapIdToSetMapping_New && id_mapping_set,
     std::vector<Grid<int>> && layers)
{
    MapRegionBuilder impl;
    for (const auto & layer : layers) {
        auto mapping_layer = id_mapping_set.make_mapping_for_layer(layer);
        for (auto & tslayer : mapping_layer) {
            auto * tileset = TileSetMappingLayer::tileset_of(tslayer.as_view());
            tileset->add_map_elements(impl, tslayer);
        }
    }
    return impl;
}

MapLoadResult MapElementCollectorState::update_progress
    (StateSwitcher & state_switcher, MapContentLoader &)
{
    MapLoadingSuccess res;
    res.loaded_region = MapRegionBuilder::
        load_from_elements(std::move(m_id_mapping_set), std::move(m_layers)).
        make_map_region(map_scale());
    state_switcher.set_next_state<ExpiredState>();
    return res;
}

/* private */ ScaleComputation MapElementCollectorState::map_scale() const {
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

/* static */ MapLoadStateMachine MapLoadStateMachine::make_with_starting_state
    (MapContentLoader & content_loader, const char * filename)
{
    MapLoadStateMachine sm;
    sm.initialize_starting_state(content_loader, filename);
    return sm;
}

void MapLoadStateMachine::initialize_starting_state
    (MapContentLoader & provider, const char * filename)
{
    auto file_contents_promise = provider.promise_file_contents(filename);
    m_state_driver.
        set_current_state<FileContentsWaitState>
        (std::move(file_contents_promise));
}

MapLoadResult MapLoadStateMachine::
    update_progress(MapContentLoader & content_loader)
{
    auto switcher = m_state_driver.state_switcher();
    auto result = m_state_driver.
        advance()->
        update_progress(switcher, content_loader);
    auto delay_required = content_loader.delay_required();
    auto advancable = m_state_driver.is_advanceable();
    if (result.is_empty() && !delay_required && advancable)
        { return update_progress(content_loader); }
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
