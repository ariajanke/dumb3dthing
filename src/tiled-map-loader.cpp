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

#include <common/StringUtil.hpp>

#include <map>
#include <unordered_map>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using tinyxml2::XMLDocument;
using TeardownTask = MapLoader::TeardownTask;

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


#   if 0
    Grid<int> layer = std::move(m_layer);
    // this call is deferred, "this" instance could be long gone
    auto tidgid_translator = std::move(m_tidgid_translater);
#   endif
    SharedPtr<TeardownTask> teardown_task = make_shared<TeardownTask>();
    auto loader = LoaderTask::make(
        [this,
         map_offset, teardown_task]
        (LoaderTask::Callbacks & callbacks)
    {
        std::vector<Entity> entities;
        auto triangles =
            add_triangles_and_link(m_layer.width(), m_layer.height(),
            [&] (Vector2I r, TrianglesAdder adder)
        {
            auto [tid, tileset] = m_tidgid_translator.gid_to_tid(m_layer(r)); {}
            auto * factory = (*tileset)(tid);
            if (!factory) return;

            EntityAndTrianglesAdder etadder{entities, adder};
            (*factory)(etadder,
                    TileFactory::NeighborInfo{tileset, m_layer, r, map_offset},
                    callbacks.platform());
        });
        entities.back().add<std::vector<TriangleLinks>>() = std::move(triangles);
        for (auto & ent : entities)
            callbacks.add(ent);
        *teardown_task = TeardownTask{std::move(entities)};
    });
    return make_tuple(loader, teardown_task);
}

/* private */ void MapLoader::add_tileset(tinyxml2::XMLElement * tileset) {
    m_tilesets.emplace_back(make_shared<TileSet>());
    m_startgids.emplace_back(tileset->IntAttribute("firstgid"));
    if (const auto * source = tileset->Attribute("source")) {
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
        XMLDocument document;
        auto contents = future->retrieve();
        document.Parse(contents.c_str());
        m_tilesets[idx]->load_information(*m_platform, document.RootElement());
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

/* private */ void MapLoader::do_content_load(std::string && contents) {
    XMLDocument document;
    if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
        // ...idk
        throw RtError{"Problem parsing XML x.x"};
    }

    auto * root = document.RootElement();
    for (auto itr = root->FirstChildElement("tileset"); itr;
         itr = itr->NextSiblingElement("tileset"))
    {
        add_tileset(itr);
    }

    auto * layer = root->FirstChildElement("layer");
    assert(layer);

    int width = layer->IntAttribute("width");
    int height = layer->IntAttribute("height");
    m_layer.set_size(width, height);
    auto * data = layer->FirstChildElement("data");
    assert(data);
    assert(!::strcmp(data->Attribute( "encoding" ), "csv"));
    auto data_text = data->GetText();
    assert(data_text);
    ; // and now I need a parsing helper for CSV strings
    Vector2I r;
    for (auto value_str : split_range(data_text, data_text + ::strlen(data_text),
                                      make_is_comma(), make_trim_whitespace()))
    {
        int tile_id = 0;
        bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), tile_id);
        assert(is_num);
        m_layer(r) = tile_id;
        r = m_layer.next(r);
    }
}
