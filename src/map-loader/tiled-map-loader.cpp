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

#include "tiled-map-loader.hpp"
#include "map-loader.hpp"
#include "../Components.hpp"
#include "../RenderModel.hpp"
#include "../Texture.hpp"
#include "TileSet.hpp"
#include "TileFactory.hpp"
#include "../PointAndPlaneDriver.hpp"

#include <ariajanke/cul/StringUtil.hpp>

#include <map>
#include <unordered_map>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

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

// std::distance requires using the same iterator type
template <typename IterBeg, typename IterEnd>
int distance(IterBeg beg, IterEnd end) {
    int i = 0;
    for (; beg != end; ++beg)
        { ++i; }
    return i;
};

inline bool is_dash(char c) { return c == '-'; }
static const auto k_whitespace_trimmer = make_trim_whitespace<const char *>();

bool is_colon(char c) { return c == ':'; }

void TiledMapRegion::request_region_load
    (const Vector2I & local_region_position,
     const SharedPtr<MapRegionPreparer> & region_preparer,
     TaskCallbacks & callbacks)
{
    Vector2I lrp{local_region_position.x % 2, local_region_position.y % 2};
    auto region_left   = std::max
        (lrp.x*m_region_size.width, 0);
    auto region_top    = std::max
        (lrp.y*m_region_size.height, 0);
    auto region_right  = std::min
        (region_left + m_region_size.width, m_factory_grid.width());
    auto region_bottom = std::min
        (region_top + m_region_size.height, m_factory_grid.height());
    if (region_left == region_right || region_top == region_bottom) {
        return;
    }
#   if 1
    auto factory_subgrid = m_factory_grid.make_subgrid(Rectangle
        {region_left, region_top,
         region_right - region_left, region_bottom - region_top});

    region_preparer->set_tile_factory_subgrid(std::move(factory_subgrid));
#   endif
    callbacks.add(region_preparer);
}

// ----------------------------------------------------------------------------

TileFactoryGrid TiledMapLoader::WaitingForFileContents::update_progress
    (StateHolder & next_state)
{
    if (!m_file_contents->is_ready()) return TileFactoryGrid{};

    std::string contents = m_file_contents->retrieve();
    Grid<int> layer;

    TiXmlDocument document;
    if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
        // ...idk
        throw RtError{"Problem parsing XML x.x"};
    }

    auto * root = document.RootElement();

    int width  = root->IntAttribute("width");
    int height = root->IntAttribute("height");

    XmlPropertiesReader propreader;
    propreader.load(root);

    TileSetsContainer tilesets_container;
    for (auto & tileset : XmlRange{root, "tileset"}) {
        add_tileset(tileset, tilesets_container);
    }

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

    set_others_stuff
        (next_state.set_next_state<WaitingForTileSets>
            (std::move(tilesets_container), std::move(layer)));

    return TileFactoryGrid{};
}

/* private */ void TiledMapLoader::WaitingForFileContents::add_tileset
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
            load_information(platform(), tileset);
    }
}

TileFactoryGrid TiledMapLoader::WaitingForTileSets::update_progress
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
        tilesets[idx]->load_information(platform(), *document.RootElement());
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
        return TileFactoryGrid{};
    }
    // no more tilesets pending
    set_others_stuff
        (next_state.set_next_state<Ready>
            (std::move(m_tidgid_translator), std::move(m_layer)));
    return TileFactoryGrid{};
}

void TileFactoryGrid::load_layer
    (const Grid<int> & gids, const GidTidTranslator & idtranslator)
{
    m_factories.set_size(gids.width(), gids.height(), nullptr);
    for (Vector2I r; r != gids.end_position(); r = gids.next(r)) {
        auto gid = gids(r);
        if (gid == 0) continue;

        auto [tid, tileset] = idtranslator.gid_to_tid(gid);
        m_factories(r) = (*tileset)(tid);
        m_tilesets.push_back(tileset);
    }
}

TileFactoryGrid TiledMapLoader::Ready::update_progress
    (StateHolder & next_state)
{
    TileFactoryGrid rv;
    rv.load_layer(m_layer, m_tidgid_translator);
    next_state.set_next_state<Expired>();
    return rv;
}

void MapLoadingDirector::on_every_frame
    (TaskCallbacks & callbacks, const Entity & physic_ent)
{
    m_active_loaders.erase
        (std::remove_if(m_active_loaders.begin(), m_active_loaders.end(),
                        [](const TiledMapLoader & loader) { return loader.is_expired(); }),
         m_active_loaders.end());

    check_for_other_map_segments(callbacks, physic_ent);
}

/* private */ void MapLoadingDirector::check_for_other_map_segments
    (TaskCallbacks & callbacks, const Entity & physics_ent)
{
    // this may turn into its own class
    // there's just so much behavior potential here

    // good enough for now
    using namespace point_and_plane;
    auto & pstate = physics_ent.get<PpState>();
    for (auto pt : { location_of(pstate), displaced_location_of(pstate) }) {
        m_region_tracker.frame_hit(to_segment_location(pt, m_chunk_size), callbacks);
    }
    m_region_tracker.frame_refresh();
}

/* private static */ Vector2I MapLoadingDirector::to_segment_location
    (const Vector & location, const Size2I & segment_size)
{
    return Vector2I
        {int(std::floor( location.x / segment_size.width )),
         int(std::floor(-location.z / segment_size.height))};
}

/* private static */ void PlayerUpdateTask::check_fall_below(Entity & ent) {
    auto * ppair = get_if<PpInAir>(&ent.get<PpState>());
    if (!ppair) return;
    auto & loc = ppair->location;
    if (loc.y < -10) {
        loc = Vector{loc.x, 4, loc.z};
        ent.get<Velocity>() = Velocity{};
    }
}
