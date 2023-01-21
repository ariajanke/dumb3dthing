/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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
#include "map-loader-helpers.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;
using State = MapLoadingState;
using WaitingForFileContents = MapLoadingWaitingForFileContents;
using WaitingForTileSets = MapLoadingWaitingForTileSets;
using Ready = MapLoadingReady;
using Expired = MapLoadingExpired;
using StateHolder = MapLoadingStateHolder;
using Rectangle = cul::Rectangle<int>;
using OptionalTileViewGrid = MapLoadingContext::OptionalTileViewGrid;

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

Grid<int> load_layer(const TiXmlElement &);

Grid<Tuple<int, SharedPtr<const TileSet>>> gid_layer_to_tid_layer
    (const Grid<int> & gids, const GidTidTranslator & gidtid_translator)
{
    Grid<Tuple<int, SharedPtr<const TileSet>>> rv;
    rv.set_size(gids.size2(), make_tuple(0, nullptr));

    for (Vector2I r; r != rv.end_position(); r = rv.next(r)) {
        // (change rt to tuple from pair)
        auto [tid, tileset] = gidtid_translator.gid_to_tid(gids(r));
        rv(r) = make_tuple(tid, tileset);
    }

    return rv;
}

struct FillerAndLocations final {
    SharedPtr<TileProducableFiller> filler;
    std::vector<TileProducableFiller::TileLocation> tile_locations;
};

std::vector<FillerAndLocations>
    tid_layer_to_fillables_and_locations
    (const Grid<Tuple<int, SharedPtr<const TileSet>>> & tids_and_tilesets)
{
    std::map<
        SharedPtr<TileProducableFiller>,
        std::vector<TileProducableFiller::TileLocation>> fillers_to_locs;
    for (Vector2I layer_loc; layer_loc != tids_and_tilesets.end_position();
         layer_loc = tids_and_tilesets.next(layer_loc))
    {
        auto [tid, tileset] = tids_and_tilesets(layer_loc);
        if (!tileset) continue;
        auto filler = tileset->find_filler(tid);
        TileProducableFiller::TileLocation tile_loc;
        tile_loc.location_on_map     = layer_loc;
        tile_loc.location_on_tileset = tileset->tile_id_to_tileset_location(tid);
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

TileGroupGrid finish_tile_group_grid
    (const std::vector<FillerAndLocations> & fillers_and_locs,
     const cul::Size2<int> & layer_size)
{
    UnfinishedTileGroupGrid unfinished_grid;
    unfinished_grid.set_size(layer_size);
    for (auto & filler_and_locs : fillers_and_locs) {
        unfinished_grid = (*filler_and_locs.filler)
            (filler_and_locs.tile_locations, std::move(unfinished_grid));
    }

    return unfinished_grid.finish();
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

Vector2I State::map_offset() const {
    verify_shared_set();
    return m_offset;
}

Rectangle State::target_tile_range() const {
    verify_shared_set();
    return m_tiles_to_load;
}

/* private */ void State::verify_shared_set() const {
    if (m_platform) return;
    throw RtError{"Unset stuff"};
}

// ----------------------------------------------------------------------------

OptionalTileViewGrid WaitingForFileContents::update_progress
    (StateHolder & next_state)
{
    if (!m_file_contents->is_ready()) return {};

    std::string contents = m_file_contents->retrieve();
    std::vector<Grid<int>> layers;

    TiXmlDocument document;
    if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
        // ...idk
        throw RtError{"Problem parsing XML x.x"};
    }

    auto * root = document.RootElement();
    XmlPropertiesReader propreader;
    propreader.load(root);

    TileSetsContainer tilesets_container;
    for (auto & tileset : XmlRange{root, "tileset"}) {
        add_tileset(tileset, tilesets_container);
    }

    for (auto & layer_el : XmlRange{root, "layer"}) {
        layers.emplace_back(load_layer(layer_el));
    }

    set_others_stuff
        (next_state.set_next_state<WaitingForTileSets>
            (std::move(tilesets_container), std::move(layers)));

    return {};
}

/* private */ void WaitingForFileContents::add_tileset
    (const TiXmlElement & tileset, TileSetsContainer & tilesets_container)
{
    tilesets_container.tilesets.emplace_back(make_shared<TileSet>());
    tilesets_container.startgids.emplace_back(tileset.IntAttribute("firstgid"));
    if (const auto * source = tileset.Attribute("source")) {
        tilesets_container.pending_tilesets.emplace_back(
            tilesets_container.tilesets.size() - 1,
            platform().promise_file_contents(source));
    } else {
        tilesets_container.tilesets.back()->
            load(platform(), tileset);
    }
}

// ----------------------------------------------------------------------------

OptionalTileViewGrid WaitingForTileSets::update_progress
    (StateHolder & next_state)
{
    // no short circuting permitted, therefore STL sequence algorithms
    // not appropriate
    auto & pending_tilesets = m_tilesets_container.pending_tilesets;
    auto & tilesets = m_tilesets_container.tilesets;
    auto & startgids = m_tilesets_container.startgids;
    static constexpr const std::size_t k_no_idx = std::string::npos;
    for (auto & [idx, future] : pending_tilesets) {
        if (!future->is_ready()) continue;
        TiXmlDocument document;
        auto contents = future->retrieve();
        document.Parse(contents.c_str());
        tilesets[idx]->load(platform(), *document.RootElement());
        idx = k_no_idx;
    }

    bool was_empty = pending_tilesets.empty();
    auto end_itr = pending_tilesets.end();
    pending_tilesets.erase(
        std::remove_if(pending_tilesets.begin(), end_itr,
            [](const Tuple<std::size_t, FutureStringPtr> & tup)
            { return std::get<std::size_t>(tup) == k_no_idx; }),
        end_itr);
    if (!was_empty && pending_tilesets.empty()) {
        m_tidgid_translator = GidTidTranslator{tilesets, startgids};
    }

    if (!pending_tilesets.empty()) {
        return {};
    }
    // no more tilesets pending
    set_others_stuff
        (next_state.set_next_state<Ready>
            (std::move(m_tidgid_translator), std::move(m_layers)));
    return {};
}

// ----------------------------------------------------------------------------

OptionalTileViewGrid TiledMapLoader::Ready::update_progress
    (StateHolder & next_state)
{
    UnfinishedProducableTileGridView unfinished_grid_view;
    for (auto & layer : m_layers) {
        auto tid_layer = gid_layer_to_tid_layer(layer, m_tidgid_translator);
        auto fillables_layer = tid_layer_to_fillables_and_locations(tid_layer);
        auto finished_group_grid = finish_tile_group_grid
            (fillables_layer, layer.size2());
        unfinished_grid_view = finished_group_grid.
            add_producables_to(std::move(unfinished_grid_view));
    }
    TileProducableViewGrid rv;
    next_state.set_next_state<Expired>();
    rv.set_layers(std::move(unfinished_grid_view), std::move(m_tidgid_translator));

    return rv;
}

// ----------------------------------------------------------------------------

bool StateHolder::has_next_state() const noexcept
    { return m_get_state; }

void StateHolder::move_state(StatePtrGetter & state_getter_ptr, StateSpace & space) {
    if (!m_get_state) {
        throw RtError{"No state to move"};
    }

    space = std::move(m_space);
    state_getter_ptr = m_get_state;
    m_get_state = nullptr;
}

// ----------------------------------------------------------------------------

OptionalTileViewGrid TiledMapLoader::update_progress() {
    StateHolder next;
    OptionalTileViewGrid rv;
    while (true) {
        rv = m_get_state(m_state_space)->update_progress(next);
        if (!next.has_next_state()) break;

        next.move_state(m_get_state, m_state_space);
        if (rv) break;
    }
    return rv;
}

namespace {

Grid<int> load_layer(const TiXmlElement & layer_el) {
    Grid<int> layer;
    layer.set_size
        (layer_el.IntAttribute("width"), layer_el.IntAttribute("height"), 0);

    auto * data = layer_el.FirstChildElement("data");
    if (!data) {
        throw RtError{"load_layer: layer must have a <data> element"};
    } else if (::strcmp(data->Attribute( "encoding" ), "csv")) {
        throw RtError{"load_layer: only csv layer data is supported"};
    }

    auto data_text = data->GetText();
    if (!data_text)
        { return layer; }

    Vector2I r;
    for (auto value_str : split_range(data_text, data_text + ::strlen(data_text),
                                      is_comma, k_whitespace_trimmer))
    {
        int tile_id = 0;
        cul::string_to_number(value_str.begin(), value_str.end(), tile_id);

        // should warn if not a number
        layer(r) = tile_id;
        r = layer.next(r);
    }
    return layer;
}

} // end of <anonymous> namespace
