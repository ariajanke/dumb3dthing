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

    std::vector<std::unique_ptr<int>> unique_pointers;
    for (int i = 0; i != 10; ++i) {
        unique_pointers.emplace_back(std::make_unique<int>(i));
    }

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
    success.loaded_region = make_unique<TiledMapRegionN>
        (unfinished_grid_view.finish(m_tidgid_translator));
#   if 0
    success.loaded_region = make_unique<TiledMapRegion>
        (unfinished_grid_view.finish(m_tidgid_translator),
         MapRegion::k_temp_region_size);
#   endif
#   if 0
    success.producables_view_grid = unfinished_grid_view.finish(m_tidgid_translator);
#   endif
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
