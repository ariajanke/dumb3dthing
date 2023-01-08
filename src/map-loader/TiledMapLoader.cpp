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
    (const Grid<int> &, const GidTidTranslator &);

auto group_tile_factories
    (const Grid<Tuple<int, SharedPtr<const TileSet>>> & tid_tiles,
     UnfinishedTileGroupGrid &&)
{
    ;
}

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

Optional<TileFactoryViewGrid> WaitingForFileContents::update_progress
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
#   if 0
    int width  = root->IntAttribute("width");
    int height = root->IntAttribute("height");
#   endif
    XmlPropertiesReader propreader;
    propreader.load(root);

    TileSetsContainer tilesets_container;
    for (auto & tileset : XmlRange{root, "tileset"}) {
        add_tileset(tileset, tilesets_container);
    }

#   if 0
    layer.set_size(width, height);
    auto * layer_el = root->FirstChildElement("layer");
    assert(layer_el);
    auto * data = layer_el->FirstChildElement("data");
    assert(data);
    assert(!::strcmp(data->Attribute( "encoding" ), "csv"));
    auto data_text = data->GetText();
    assert(data_text);
    ; // and now I need a parsing helper for CSV strings
    Vector2I r;
    for (auto value_str : split_range(data_text, data_text + ::strlen(data_text),
                                      is_comma, k_whitespace_trimmer))
    {
        int tile_id = 0;
        bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), tile_id);
        assert(is_num);
        layer(r) = tile_id;
        r = layer.next(r);
    }
#   else
    for (auto & layer_el : XmlRange{root, "layer"}) {
        layers.emplace_back(load_layer(layer_el));
    }
#   endif

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

Optional<TileFactoryViewGrid> WaitingForTileSets::update_progress
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

Optional<TileFactoryViewGrid> TiledMapLoader::Ready::update_progress
    (StateHolder & next_state)
{
    TileFactoryViewGrid rv;
    rv.load_layers(m_layers, std::move(m_tidgid_translator));
    next_state.set_next_state<Expired>();
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

Optional<TileFactoryViewGrid> TiledMapLoader::update_progress() {
    StateHolder next;
    Optional<TileFactoryViewGrid> rv;
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
