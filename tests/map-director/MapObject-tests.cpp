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

#include "../../src/map-director/MapObject.hpp"

#include "../../src/map-director/ViewGrid.hpp"

#include "../test-helpers.hpp"

#include <tinyxml2.h>

constexpr const auto k_simple_object_map =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<map version=\"1.10\" tiledversion=\"1.10.2\" orientation=\"orthogonal\" "
     "renderorder=\"right-down\" width=\"32\" height=\"32\" tilewidth=\"32\" "
     "tileheight=\"32\" infinite=\"0\" nextlayerid=\"6\" nextobjectid=\"8\">"
"<objectgroup id=\"2\" name=\"Object Layer 1\" class=\"immediate\">"
  "<object id=\"1\" type=\"player-spawn-point\" x=\"426.604\" y=\"452.697\" name=\"player\">"
   "<properties>"
    "<property name=\"elevation\" value=\"10\"/>"
   "</properties>"
   "<point/>"
  "</object>"
 "</objectgroup>"
"</map>";

constexpr const auto k_object_map_for_name_finding =
"";

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe("MapObjectGroup")([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    assert(node);
    MapObjectCollection collection;
    collection.load(*node);
    auto * group = collection.top_level_group();
    assert(group);
    mark_it("can find player by name", [&] {
        return test_that(group->seek_by_name("player"));
    });
});

describe<MapObjectCollection>("MapObjectCollection").depends_on<MapObject>()([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    assert(node);
    MapObjectCollection collection;
    collection.load(*node);
    static constexpr const int k_player_id = 1;
    auto * player_object = collection.seek_object_by_id(k_player_id);
    mark_it("able to find player object", [&] {
        return test_that(player_object);
    }).
    mark_it("has a top level group", [&] {
        return test_that(collection.top_level_group());
    }).
    mark_it("top level group has player object accessible", [&] {
        auto * group = collection.top_level_group();
        if (!group)
            { return test_that(false); }
        auto objects = group->objects();
        auto itr = std::find_if(objects.begin(), objects.end(),
                     [&](const MapObject & object)
                     { return object.id() == k_player_id; });
        if (itr == objects.end())
            { return test_that(false); }
        return test_that(itr->id() == k_player_id);
    });
});

describe<MapObject>("MapObject").depends_on<MapObject::CStringHasher>()([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    auto objectgroup = (**node).FirstChildElement("objectgroup");
    auto object_el = objectgroup->FirstChildElement("object");
    MapObjectGroup group{1};
    auto object = MapObject::load_from(*object_el, group, MapObjectRetrieval::null_instance());
    assert(node);
    mark_it("parses object id", [&] {
        return test_that(object.id() == 1);
    }).
    mark_it("parses object name", [&] {
        return test_that(object.name().value_or("") == std::string{"player"});
    }).
    mark_it("parses a property correctly", [&] {
        auto elevation = object.get_property<int>("elevation");
        return test_that(elevation.value_or(0) == 10);
    });
});

describe<MapObject::CStringHasher>("CStringHasher")([] {
    mark_it("consistently hashes strings", [&] {
        auto sample_hash1 = MapObject::CStringHasher{}("sample");
        auto strings = { "apples", "peas", "car", "reallylongstring", "stuff" };
        for (auto & string : strings) {
            (void)MapObject::CStringHasher{}(string);
        }
        auto sample_hash2 = MapObject::CStringHasher{}("sample");
        return test_that(sample_hash1 == sample_hash2);
    });
});

return [] {};

} ();
