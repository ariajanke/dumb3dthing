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

namespace {

using namespace cul::exceptions_abbr;
using State = MapLoadingState;
using WaitingForFileContents = MapLoadingWaitingForFileContents;
using WaitingForTileSets = MapLoadingWaitingForTileSets;
using Ready = MapLoadingReady;
using Expired = MapLoadingExpired;
using StateHolder = MapLoadingStateHolder;
using MapLoadResult = MapLoadingContext::MapLoadResult;

template <typename Key, typename Value, typename Comparator, typename Key2, typename Func>
void on_key_found(const std::map<Key, Value, Comparator> & map, const Key2 & key, Func && f_) {
    auto itr = map.find(key);
    if (itr == map.end()) return;
    f_(itr->second);
}

template <typename Key, typename Value, typename Comparator, typename Key2>
Value find_key(const std::map<Key, Value, Comparator> & map, const Key2 & key, const Value & default_val) {
    auto itr = map.find(key);
    if (itr == map.end()) return default_val;
    return itr->second;
}

template <typename Key, typename Comparator, typename Key2>
const char * find_key_cstr(const std::map<Key, const char *, Comparator> & map, const Key2 & key) {
    return find_key(map, key, static_cast<const char *>(nullptr));
}

class XmlPropertiesReader final {
public:
    XmlPropertiesReader() {}

    void load(const TiXmlElement * el) {
        for (auto & pair : m_properties) {
            pair.second = nullptr;
        }
        const auto props = XmlRange{el ? el->FirstChildElement("properties") : el, "property"};
        for (auto & property : props) {
           auto * name = property.Attribute("name");
           if (!name) continue;

           m_properties[std::string{name}] = property.Attribute("value");
       }
    }

    const char * value_for(const char * key) const
        { return find_key_cstr(m_properties, key); }

private:
    std::map<std::string, const char *> m_properties;
};

static const auto k_whitespace_trimmer = make_trim_whitespace<const char *>();

Either<MapLoadingWarningEnum, Grid<int>> load_layer_(const TiXmlElement &);

Grid<Tuple<int, SharedPtr<const TileSet>>> gid_layer_to_tid_layer
    (const Grid<int> & gids, const TileMapIdToSetMapping & gidtid_translator)
{
    Grid<Tuple<int, SharedPtr<const TileSet>>> rv;
    rv.set_size(gids.size2(), make_tuple(0, nullptr));

    for (Vector2I r; r != rv.end_position(); r = rv.next(r)) {
        // (change rt to tuple from pair)
        auto [tid, tileset] = gidtid_translator.map_id_to_set(gids(r));
        rv(r) = make_tuple(tid, tileset);
    }

    return rv;
}

struct FillerAndLocations final {
    SharedPtr<ProducableGroupFiller> filler;
    std::vector<TileLocation> tile_locations;
};

std::vector<FillerAndLocations>
    tid_layer_to_fillables_and_locations
    (const Grid<Tuple<int, SharedPtr<const TileSet>>> & tids_and_tilesets)
{
    std::map<
        SharedPtr<ProducableGroupFiller>,
        std::vector<TileLocation>> fillers_to_locs;
    for (Vector2I layer_loc; layer_loc != tids_and_tilesets.end_position();
         layer_loc = tids_and_tilesets.next(layer_loc))
    {
        auto [tid, tileset] = tids_and_tilesets(layer_loc);
        if (!tileset) continue;
        auto filler = tileset->find_filler(tid);
        TileLocation tile_loc;
        tile_loc.on_map     = layer_loc;
        tile_loc.on_tileset = tileset->tile_id_to_tileset_location(tid);
        fillers_to_locs[filler].push_back(tile_loc);
    }


    std::vector<FillerAndLocations> rv;
    rv.reserve(fillers_to_locs.size());
    for (auto & [filler, locs] : fillers_to_locs) {
        FillerAndLocations filler_and_locs;
        filler_and_locs.filler = filler;
        filler_and_locs.tile_locations = std::move(locs);
        rv.emplace_back(std::move(filler_and_locs));
    }
    return rv;
}

ProducableGroupTileLayer make_unfinsihed_tile_group_grid
    (const std::vector<FillerAndLocations> & fillers_and_locs,
     const Size2I & layer_size)
{
    ProducableGroupTileLayer unfinished_grid;
    unfinished_grid.set_size(layer_size);
    for (auto & filler_and_locs : fillers_and_locs) {
        if (!filler_and_locs.filler) continue;
        unfinished_grid = (*filler_and_locs.filler)
            (filler_and_locs.tile_locations, std::move(unfinished_grid));
    }

    return unfinished_grid;
}


// grid ids, to filler groups and specific types
// groups provide specific factory types
// what's difficult here:
// I need to instantiate specificly typed factories, much like tileset does now
// Each group will have to do this for a subset of tileset.
//

} // end of <anonymous> namespace

Platform & State::platform() const {
    verify_shared_set();
    return *m_platform;
}

/* private */ void State::verify_shared_set() const {
    if (m_platform) return;
    throw RtError{"Unset stuff"};
}

// ----------------------------------------------------------------------------

MapLoadResult WaitingForFileContents::update_progress
    (StateHolder & next_state)
{
    using namespace map_loading_messages;
    using Lost = Future<std::string>::Lost;

    return (*m_file_contents)().map_left([] (Lost) {
        return MapLoadingError{k_tile_map_file_contents_not_retrieved};
    }).
    chain([this, &next_state] (std::string && contents) {
        TiXmlDocument document;
        if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
            // ...idk
            return MapLoadResult{ MapLoadingError{k_tile_map_file_contents_not_retrieved} };
        }

        auto * root = document.RootElement();
        XmlPropertiesReader propreader;
        propreader.load(root);

        UnfinishedTileSetCollection tilesets_container{warnings_adder()};
        for (auto & tileset : XmlRange{root, "tileset"}) {
            add_tileset(tileset, tilesets_container);
        }

        std::vector<Grid<int>> layers;
        for (auto & layer_el : XmlRange{root, "layer"}) {
            auto ei = load_layer_(layer_el);
            if (ei.is_left()) {
                warnings_adder().add(ei.left());
            } else if (ei.is_right()) {
                layers.emplace_back(ei.right());
            }
        }

        set_others_stuff
            (next_state.set_next_state<WaitingForTileSets>
                (tilesets_container.finish(), std::move(layers)));
        return MapLoadResult{};
    });
}

/* private */ void WaitingForFileContents::add_tileset
    (const TiXmlElement & tileset,
     UnfinishedTileSetCollection & collection)
{
    using namespace cul::either;
    using namespace map_loading_messages;
    constexpr const int k_no_first_gid = -1;

    auto first_gid = tileset.IntAttribute("firstgid", k_no_first_gid);
    if (first_gid == k_no_first_gid) {
        return collection.add(k_invalid_tile_data);
    }

    if (const auto * source = tileset.Attribute("source")) {
        return collection.add(first_gid, platform().promise_file_contents(source));
    } else {
        // platform(), tileset
        auto tileset_ = make_shared<TileSet>();
        tileset_->load(platform(), tileset);
        return collection.add(first_gid, std::move(tileset_));
    }
}

// ----------------------------------------------------------------------------

MapLoadResult WaitingForTileSets::update_progress
    (StateHolder & next_state)
{
    // no short circuting permitted, therefore STL sequence algorithms
    // not appropriate
    auto res = m_tilesets_container->attempt_finish(platform());
    if (!res) return {};

    set_others_stuff
        (next_state.set_next_state<Ready>
            (std::move(*res), std::move(m_layers)));
    return {};
}

// ----------------------------------------------------------------------------

MapLoadResult TiledMapLoader::Ready::update_progress
    (StateHolder & next_state)
{
    UnfinishedProducableTileViewGrid unfinished_grid_view;
    for (auto & layer : m_layers) {
        auto tid_layer = gid_layer_to_tid_layer(layer, m_tidgid_translator);
        auto fillables_layer = tid_layer_to_fillables_and_locations(tid_layer);
        unfinished_grid_view = make_unfinsihed_tile_group_grid(fillables_layer, layer.size2()).
            move_self_to(std::move(unfinished_grid_view));
    }
    next_state.set_next_state<Expired>();

    MapLoadingSuccess success;
    success.loaded_region = make_unique<TiledMapRegion>
        (unfinished_grid_view.finish(m_tidgid_translator));
    return success;
}

// ----------------------------------------------------------------------------

bool StateHolder::has_next_state() const noexcept
    { return m_get_state; }

void StateHolder::move_state(StatePtrGetter & state_getter_ptr, StateSpace & space) {
    if (!m_get_state) {
        throw RtError{"No state to move"};
    }

    space.swap(m_space);
    state_getter_ptr = m_get_state;
    m_get_state = nullptr;
}

Tuple<StateHolder::StatePtrGetter, StateHolder::StateSpace>
    StateHolder::move_out_state()
{
    if (!m_get_state) {
        throw RtError{"No state to move"};
    }
    auto getter = m_get_state;
    auto space = std::move(m_space);
    m_space = MapLoadingExpired{};
    m_get_state = nullptr;
    return make_tuple(getter, std::move(space));
}

// ----------------------------------------------------------------------------

MapLoadResult TiledMapLoader::update_progress() {
    StateHolder next;
    MapLoadResult rv;
    while (true) {
        rv = m_get_state(m_state_space)->update_progress(next);
        if (!next.has_next_state()) break;

        next.move_state(m_get_state, m_state_space);
        if (!rv.is_empty()) break;
    }
    return rv;
}

// ----------------------------------------------------------------------------
// all the new shit

namespace tiled_map_loading {

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

using TileSetAndStartGid = TileMapIdToSetMapping::TileSetAndStartGid;

/* static */ std::vector<TileSetAndStartGid>
    ProducableLoadState::convert_to_tileset_and_start_gids
    (std::vector<TileSetContent> && tileset_contents,
     Platform & platform)
{
    std::vector<TileSetAndStartGid> tilesets_and_start_gids;
    tilesets_and_start_gids.reserve(tileset_contents.size());
    for (auto & contents : tileset_contents) {
        auto tileset = make_shared<TileSet>();
        tileset->load(platform, contents.element());
        tilesets_and_start_gids.emplace_back
            (std::move(tileset), contents.first_gid());
    }
    return tilesets_and_start_gids;
}

/* static */ ProducableTileViewGrid
    ProducableLoadState::make_producable_view_grid
    (std::vector<Grid<int>> && layers,
     TileMapIdToSetMapping && gid_to_tid_mapping)
{
    UnfinishedProducableTileViewGrid unfinished_grid_view;
    for (auto & layer : layers) {
        auto tid_layer = gid_layer_to_tid_layer(layer, gid_to_tid_mapping);
        auto fillables_layer = tid_layer_to_fillables_and_locations(tid_layer);
        unfinished_grid_view = make_unfinsihed_tile_group_grid(fillables_layer, layer.size2()).
            move_self_to(std::move(unfinished_grid_view));
    }
    return unfinished_grid_view.finish(gid_to_tid_mapping);
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
    MapLoadingSuccess success;
    success.loaded_region =
        make_unique<TiledMapRegion>(make_producable_view_grid());
    switcher.set_next_state<ExpiredState>();
    return success;
}

/* private */ TileMapIdToSetMapping ProducableLoadState::make_tidgid_mapping() {
    return TileMapIdToSetMapping
        {convert_to_tileset_and_start_gids(std::move(m_finished_contents),
         platform())};
}

/* private */ ProducableTileViewGrid
    ProducableLoadState::make_producable_view_grid()
{
    return make_producable_view_grid
        (std::move(m_layers), make_tidgid_mapping());
}

} // end of tiled_map_loading namespace

// end of new shit
// ----------------------------------------------------------------------------

namespace {

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
