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
#include "Components.hpp"
#include "RenderModel.hpp"
#include "Texture.hpp"
#include "TileSet.hpp"
#include "TileFactory.hpp"

#include <common/StringUtil.hpp>

#include <map>
#include <unordered_map>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using TeardownTask = MapLoader::TeardownTask;
using MapSide = MapEdgeLinks::Side;
using TileRange = MapEdgeLinks::TileRange;

} // end of <anonymous> namespace

Tuple<SharedPtr<LoaderTask>, SharedPtr<TeardownTask>> MapLoader::operator ()
    (const Vector2I & map_offset)
{
    if (m_file_contents) { if (m_file_contents->is_ready()) {
        do_content_load(m_file_contents->retrieve());
        m_file_contents = nullptr;
    }}
    if (check_has_remaining_pending_tilesets())
        { return make_tuple(nullptr, nullptr); }
    if (m_file_contents || !m_pending_tilesets.empty())
        { return make_tuple(nullptr, nullptr); }

    SharedPtr<TeardownTask> teardown_task = make_shared<TeardownTask>();
    auto loader = LoaderTask::make(
        [this, map_offset, teardown_task]
        (LoaderTask::Callbacks & callbacks)
    {
        std::vector<Entity> entities;
        auto triangles_and_grid =
            add_triangles_and_link_(m_layer.width(), m_layer.height(),
            [&] (Vector2I r, TrianglesAdder adder)
        {
            auto [tid, tileset] = m_tidgid_translator.gid_to_tid(m_layer(r)); {}
            auto * factory = (*tileset)(tid);
            if (!factory) return;

            class Impl final : public EntityAndTrianglesAdder {
            public:
                Impl(std::vector<Entity> & entities,
                     TrianglesAdder & triangles_):
                    m_tri_adder(triangles_), m_entities(entities) {}

                void add_triangle(const TriangleSegment & triangle) final
                    { m_tri_adder.add_triangle( make_shared<TriangleSegment>(triangle) ); }

                void add_entity(const Entity & ent) final { m_entities.push_back(ent); }

            private:
                TrianglesAdder & m_tri_adder;
                std::vector<Entity> & m_entities;
            };

            class GridIntfImpl final : public SlopesGridInterface {
            public:
                GridIntfImpl(const GidTidTranslator & translator, const Grid<int> & grid):
                    m_translator(translator), m_grid(grid) {}

                Slopes operator () (Vector2I r) const {
                    static const Slopes k_all_inf{0, k_inf, k_inf, k_inf, k_inf};
                    if (!m_grid.has_position(r)) return k_all_inf;
                    auto [tid, tileset] = m_translator.gid_to_tid(m_grid(r));
                    auto factory = (*tileset)(tid);
                    if (!factory) return k_all_inf;
                    return factory->tile_elevations();
                }

            private:
                const GidTidTranslator & m_translator;
                const Grid<int> & m_grid;
            };

            GridIntfImpl gridintf{m_tidgid_translator, m_layer};
            Impl etadder{entities, adder};
            (*factory)(etadder, TileFactory::NeighborInfo{gridintf, r, map_offset},
                    // TileFactory::NeighborInfo{tileset, m_layer, r, map_offset},
                    callbacks.platform());
        });
        entities.back().add<
            std::vector<TriangleLinks>, Grid<cul::View<std::vector<TriangleLinks>::const_iterator>>>()
            = std::move(triangles_and_grid);
        for (auto & ent : entities)
            callbacks.add(ent);
        *teardown_task = TeardownTask{std::move(entities)};
    });
    return make_tuple(loader, teardown_task);
}

/* private */ void MapLoader::add_tileset(const tinyxml2::XMLElement & tileset) {
    m_tilesets.emplace_back(make_shared<TileSet>());
    m_startgids.emplace_back(tileset.IntAttribute("firstgid"));
    if (const auto * source = tileset.Attribute("source")) {
        m_pending_tilesets.emplace_back(
            m_tilesets.size() - 1,
            m_platform->promise_file_contents(source));
    } else {
        m_tilesets.back()->load_information(*m_platform, tileset);
    }
}

/* private */ bool MapLoader::check_has_remaining_pending_tilesets() {
    // no short circuting permitted, therefore STL sequence algorithms
    // not appropriate
    static constexpr const std::size_t k_no_idx = std::string::npos;
    for (auto & [idx, future] : m_pending_tilesets) {
        if (!future->is_ready()) continue;
        TiXmlDocument document;
        auto contents = future->retrieve();
        document.Parse(contents.c_str());
        m_tilesets[idx]->load_information(*m_platform, *document.RootElement());
        idx = k_no_idx;
    }
    bool was_empty = m_pending_tilesets.empty();
    auto end_itr = m_pending_tilesets.end();
    m_pending_tilesets.erase(
        std::remove_if(m_pending_tilesets.begin(), end_itr,
            [](const Tuple<std::size_t, FutureStringPtr> & tup)
            { return std::get<std::size_t>(tup) == k_no_idx; }),
        end_itr);
    if (!was_empty && m_pending_tilesets.empty()) {
        m_tidgid_translator = GidTidTranslator{m_tilesets, m_startgids};
    }
    return !m_pending_tilesets.empty();
}

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

TileRange to_range
    (const TileRange & range, const cul::View<const char *> & arg_)
{
    constexpr const auto k_not_num = "must be numeric, and ";
    if (std::equal(arg_.begin(), arg_.end(), "whole")) {
        return range;
    }

    auto args = split_range_with_index(arg_.begin(), arg_.end(), is_dash, k_whitespace_trimmer);
    constexpr const int k_uninit = -1;
    int start = k_uninit;
    int end = k_uninit;
    for (auto [arg, idx] : args) {
        switch (idx) {
        case 0: case 1: {
            // I will have to fix cul
            auto enditr = arg.begin();
            for (; enditr != arg.end(); ++enditr) {}
            int & out = idx == 0 ? start : end;
            if (!cul::string_to_number(arg.begin(), enditr, out)) {
                throw RtError{k_not_num};
            }
            }
            break;
        default: throw RtError{"must be exactly two"};
        }
    }
    auto dir = normalize(range.end_location() - range.begin_location());
    return TileRange{ range.begin_location() + dir*start,
                      range.begin_location() + dir*end  };
}

bool is_colon(char c) { return c == ':'; }

/* private */ void MapLoader::do_content_load(std::string && contents) {
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

    std::vector<MapEdgeLinks::MapLinks> links;

    const auto k_link_ranges = {
        // mmm... will I also need to capture how each side will displace
        // the map?
        make_tuple("loader:north", TileRange{Vector2I{}, Vector2I{width, 0}}, MapSide::north),
        make_tuple("loader:south", TileRange{Vector2I{0, height}, Vector2I{width, height}}, MapSide::south),
        make_tuple("loader:east" , TileRange{Vector2I{width, 0}, Vector2I{width, height}}, MapSide::east),
        make_tuple("loader:west" , TileRange{Vector2I{0, 0}, Vector2I{0, height}}, MapSide::west),
    };

    static auto add_tile_range = []
        (std::vector<MapEdgeLinks::MapLinks> & links, const cul::View<const char *> & arg, const TileRange & original_range)
    {
        auto args = split_range(arg.begin(), arg.end(), is_comma, make_trim_whitespace<const char *>());
        auto count = distance(args.begin(), args.end());
        links.reserve(count);
        if (count == 0) {
            throw RtError{"must be at least one tile range"};
        }
        for (auto trange_str : args) {
            links.emplace_back();
            links.back().range = to_range(original_range, trange_str);
        }
    };

    static auto add_filenames = []
        (std::vector<MapEdgeLinks::MapLinks> & links, const cul::View<const char *> & arg)
    {
        auto args = split_range(arg.begin(), arg.end(), is_comma, k_whitespace_trimmer);
        auto count = distance(args.begin(), args.end());
        if (count == 1) {
            auto fn = view_to_string(*args.begin());
            for (auto & link : links) {
                link.filename = fn;
            }
        } else if (count == int(links.size())) {
            auto arg_itr = args.begin();
            for (auto & link : links) {
                link.filename = view_to_string(*arg_itr);
                ++arg_itr;
            }
        } else {
            throw RtError{"number of filenames must be either one, or number of ranges"};
        }
    };

    for (auto [link_name, range, side] : k_link_ranges) {
        if (const char * value = propreader.value_for(link_name)) {
            for (auto [arg, arg_num] : split_range_with_index(value, value + ::strlen(value),
                                        is_colon, k_whitespace_trimmer))
            {
                switch (arg_num) {
                // tile range(s)
                case 0: add_tile_range(links, arg, range); break;
                // filename(s)
                case 1: add_filenames(links, arg); break;
                default: throw RtError{"only two arguments permitted"};
                }
            }
            m_links.set_side(side, links);
            links.clear();
        }
    }

    for (auto & tileset : XmlRange{root, "tileset"}) {
        add_tileset(tileset);
    }

    m_layer.set_size(width, height);
    auto * layer = root->FirstChildElement("layer");
    assert(layer);
    auto * data = layer->FirstChildElement("data");
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
        m_layer(r) = tile_id;
        r = m_layer.next(r);
    }
}
