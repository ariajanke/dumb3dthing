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
#include "TilesetBase.hpp"
#include "CompositeTileset.hpp"

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

/* static */ Optional<TilesetLoadersWithStartGid>
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
        return TilesetLoadersWithStartGid
            {first_gid,
             TilesetLoadingTask::begin_loading(source, content_loader)};
    } else {
        return TilesetLoadersWithStartGid
            {first_gid,
             TilesetLoadingTask::begin_loading(DocumentOwningNode{tileset})};
    }
}

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

/* static */ std::vector<TilesetLoadersWithStartGid>
    InitialDocumentReadState::load_future_tilesets
    (const DocumentOwningNode & document_root,
     MapContentLoader & content_loader)
{
    std::vector<TilesetLoadersWithStartGid> future_tilesets;
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
    (std::vector<TilesetLoadersWithStartGid> && tileset_load,
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
            auto task_ptr = std::make_shared<TilesetLoadingTask>
                (std::move(future.other));
            content_loader.wait_on(task_ptr);
            split.future_tilesets.emplace_back(start_gid, std::move(task_ptr));
        } else {
            (void)res.require().
                map([start_gid, &split] (SharedPtr<TilesetBase> && tileset)
            {
                split.ready_tilesets.emplace_back(start_gid, std::move(tileset));
                return 0;
            });
        }
    }
    return split;
}

MapLoadResult InitialDocumentReadState::update_progress
    (StateSwitcher & switcher, MapContentLoader & content_loader)
{
    auto layers = load_layers(*m_document_root, content_loader);
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

/* static */ std::vector<TilesetWithStartGid>
    TileSetLoadState::finish_tilesets
    (std::vector<TilesetProviderWithStartGid> && future_tilesets,
     std::vector<TilesetWithStartGid> && ready_tilesets)
{
    ready_tilesets.reserve(ready_tilesets.size() + future_tilesets.size());
    for (auto & future : future_tilesets) {
        auto start_gid = future.start_gid;
        (void)future.other->retrieve().require().map(
            [&ready_tilesets, start_gid] (SharedPtr<TilesetBase> && tileset)
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
     std::vector<TilesetProviderWithStartGid> && future_tilesets_,
     std::vector<TilesetWithStartGid> && ready_tilesets_):
    m_document_root(std::move(document_root_)),
    m_layers(std::move(layers_)),
    m_future_tilesets(std::move(future_tilesets_)),
    m_ready_tilesets(std::move(ready_tilesets_)) {}

MapLoadResult TileSetLoadState::update_progress
    (StateSwitcher & state_switcher, MapContentLoader &)
{
    auto finished_tilesets = finish_tilesets
        (std::move(m_future_tilesets), std::move(m_ready_tilesets));
    state_switcher.set_next_state<MapElementCollectorState>
        (std::move(m_document_root),
         TileMapIdToSetMapping{ std::move(finished_tilesets) },
         std::move(m_layers));
    return {};
}

// ----------------------------------------------------------------------------

MapElementCollectorState::MapElementCollectorState
    (DocumentOwningNode && document_root_,
     TileMapIdToSetMapping && mapping_,
     std::vector<Grid<int>> && layers_):
    m_document_root(std::move(document_root_)),
    m_id_mapping_set(std::move(mapping_)),
    m_layers(std::move(layers_)) {}

class MapRegionBuilder final : public TilesetMapElementCollector {
public:
    static MapRegionBuilder
        load_from_elements(TileMapIdToSetMapping && id_mapping_set,
                           std::vector<Grid<int>> && layers);

    MapRegionBuilder() {}

    void add(StackableProducableTileGrid && layer) final {
        m_producables_stacker = layer.
            stack_with(std::move(m_producables_stacker));
    }

    void add(StackableSubRegionGrid && subregion_layer) final {
        m_subregion_stacker = subregion_layer.
            stack_with(std::move(m_subregion_stacker));
    }

    UniquePtr<MapRegion> make_map_region(ScaleComputation && scale) {
        if (m_subregion_stacker.is_empty()) {
            return std::make_unique<TiledMapRegion>
                (m_producables_stacker.to_producables(), std::move(scale));
        }
        return std::make_unique<CompositeMapRegion>
            (m_subregion_stacker.to_sub_region_view_grid(),
             std::move(scale));
    }

private:
    ProducableTileGridStacker m_producables_stacker;
    SubRegionGridStacker m_subregion_stacker;
};

/* static */ MapRegionBuilder
    MapRegionBuilder::load_from_elements
    (TileMapIdToSetMapping && id_mapping_set,
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
