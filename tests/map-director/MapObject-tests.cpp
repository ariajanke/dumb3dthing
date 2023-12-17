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
#include "../../src/map-director/MapObjectCollection.hpp"

#include "../test-helpers.hpp"

#include <tinyxml2.h>

namespace {

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

// this turns out to not be valid TilEd map
// but were created just to make sure the "find up the tree" feature works
//
// as there maybe a way to make that thing actually happen sometime in the
// future
constexpr const auto k_object_up_tree_example =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<map version=\"1.10\" tiledversion=\"1.10.2\">"
    "<objectgroup id=\"2\">"
    "  <object id=\"1\" name=\"x\"></object>"
    "  <objectgroup id=\"3\">"
    "    <object id=\"2\" name=\"x\"></object>"
    "    <objectgroup id=\"4\" name=\"deeply-nested\">"
    "      <object id=\"3\" name=\"something\"></object>"
    "    </objectgroup>"
    "  </objectgroup>"
    "</objectgroup>"
    "</map>";

constexpr const auto k_groups_for_bfs_example =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<map version=\"1.10\" tiledversion=\"1.10.2\">"
    "<objectgroup id=\"1\">"
    "    <objectgroup id=\"2\">"
    "        <objectgroup id=\"4\"></objectgroup>"
    "        <objectgroup id=\"5\"></objectgroup>"
    "    </objectgroup>"
    "    <objectgroup id=\"3\">"
    "        <objectgroup id=\"6\"></objectgroup>"
    "        <objectgroup id=\"7\"></objectgroup>"
    "    </objectgroup>"
    "</objectgroup>"
    "</map>";

constexpr const auto k_multiple_top_level_groups =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<map>"
    "<objectgroup id=\"2\" name=\"Object Layer 1\" class=\"immediate\">"
      "<object id=\"1\" type=\"player-spawn-point\" x=\"426.604\" y=\"452.697\">"
      "</object>"
    "</objectgroup>"
    "<group id=\"4\" name=\"Group Layer 1\" class=\"immediate\">"
      "<objectgroup id=\"5\" name=\"wave-one\" class=\"stop\">"
        "<object id=\"5\" type=\"baddie-type-a\" x=\"530.979\" y=\"398.083\">"
        "</object>"
        "<object id=\"6\" type=\"baddie-type-a\" x=\"514.594\" y=\"438.134\">"
        "</object>"
        "<object id=\"7\" type=\"baddie-type-a\" x=\"561.927\" y=\"437.527\">"
        "</object>"
      "</objectgroup>"
    "</group>"
    "</map>";

constexpr const auto k_object_with_properties =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<map>"
    "<objectgroup id=\"2\" name=\"Object Layer 1\">"
      "<object id=\"1\" type=\"player-spawn-point\" x=\"426.604\" y=\"452.697\""
        "      someattribute=\"hello\">"
        "<properties>"
          "<property name=\"object\" value=\"3\"/>"
          "<property name=\"group\" value=\"2\"/>"
          "<property name=\"numeric\" value=\"10\"/>"
          "<property name=\"string\" value=\"hello mario\"/>"
        "</properties>"
      "</object>"
      "<object id=\"3\" name=\"something\"></object>"
    "</objectgroup>"
    "</map>";

struct MapObjectFindUpTree final {};

using CStringEqual = MapObject::CStringEqual;
using GroupContainer = MapObject::GroupContainer;

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe("MapObjectGroup")([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    assert(node);
    auto collection = MapObjectCollection::load_from(*node);
    auto & group = *collection.top_level_groups().begin();
    mark_it("can find player by name", [&] {
        return test_that(group.seek_by_name("player"));
    });
});

describe("MapObjectGroup::initialize_names_and_parents_for_map")([] {
    auto node = DocumentOwningNode::load_root(k_multiple_top_level_groups);
    assert(node);
    auto groups = std::get<std::vector<MapObjectGroup>>(MapObjectGroup::initialize_for_map(*node));
    // two groups without a parent is completely valid
    mark_it("loads three groups", [&] {
        return test_that(groups.size() == 3);
    }).
    mark_it("all but the first group have a parent", [&] {
        // I want a fast fall option...
        if (groups.size() != 3)
            { return test_that(false); }
        bool first_two_non_parents = std::all_of
            (groups.begin(),
             groups.begin() + 2,
             [] (const MapObjectGroup & group)
             { return !group.has_parent(); });
        return test_that(groups.back().has_parent() &&
                         first_two_non_parents);
    });
});

describe("MapObjectGroup::initialize_names_and_parents_for_map")([] {
    auto node = DocumentOwningNode::load_root(k_groups_for_bfs_example);
    assert(node);
    auto groups = std::get<GroupContainer>(MapObjectGroupForTests::initialize_names_and_parents_for_map(*node));
    mark_it("loads seven groups", [&] {
        return test_that(groups.size() == 7);
    }).
    mark_it("groups are in BFS order", [&] {
        auto ids = { 1, 2, 3, 4, 5, 6, 7 };
        auto are_equal_and_in_order = std::equal
            (groups.begin(), groups.end(), ids.begin(), ids.end(),
             [](const MapObjectGroup & group, int id)
             { return group.id() == id; });
        return test_that(are_equal_and_in_order);
    });
});

describe("MapObjectGroup::initialize_names_and_parents_for_map")([] {
    auto node = DocumentOwningNode::load_root(k_object_up_tree_example);
    assert(node);
    auto groups = std::get<GroupContainer>(MapObjectGroupForTests::initialize_names_and_parents_for_map(*node));
    mark_it("loads three groups", [&] {
        return test_that(groups.size() == 3);
    }).
    mark_it("contains \"deeply-nested\" group", [&] {
        bool found_nested_group = std::any_of
            (groups.begin(),
             groups.end(),
             [] (const MapObjectGroup & group)
             { return  CStringEqual{}(group.name(), "deeply-nested"); });
        return test_that(found_nested_group);
    });
});

describe<MapObjectCollection>("MapObjectCollection").depends_on<MapObject>()([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    assert(node);
    auto collection = MapObjectCollection::load_from(*node);

    static constexpr const int k_player_id = 1;
    auto * player_object = collection.seek_object_by_id(k_player_id);
    mark_it("able to find player object", [&] {
        return test_that(player_object);
    }).
    mark_it("has a top level group", [&] {
        auto groups = collection.top_level_groups();
        auto group_count = groups.end() - groups.begin();
        return test_that(group_count > 0);
    }).
    mark_it("top level group has player object accessible", [&] {
        auto group = collection.top_level_groups().begin();
        if (group == collection.top_level_groups().end())
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

describe<MapObjectFindUpTree>("MapObject#seek_by_object_name").
    depends_on<MapObjectCollection>()([]
{
    auto node = DocumentOwningNode::load_root(k_object_up_tree_example);
    assert(node);
    auto collection = MapObjectCollection::load_from(*node);
    auto * object = collection.seek_object_by_id(3);
    mark_it("able to find object id=3", [&] {
        return test_that(object);
    }).
    mark_it("object can see two objects", [&] {
        auto x = object->seek_by_object_name("x");
        auto something = object->seek_by_object_name("something");
        return test_that(x && something);
    }).
    mark_it("finds the right object", [&] {
        if (!object)
            { return test_that(false); }
        auto * found = object->seek_by_object_name("x");
        if (!found)
            { return test_that(false); }
        return test_that(found->id() == 2);
    });
});

describe<MapObject>("MapObject").depends_on<MapObject::CStringHasher>()([] {
    auto node = DocumentOwningNode::load_root(k_simple_object_map);
    auto objectgroup = (**node).FirstChildElement("objectgroup");
    auto object_el = objectgroup->FirstChildElement("object");
    MapObjectGroup group{1};
    auto object = MapObject::load_from(*object_el, group);
    assert(node);
    mark_it("parses object id", [&] {
        return test_that(object.id() == 1);
    }).
    mark_it("parses object name", [&] {
        return test_that(object.name() == std::string{"player"});
    }).
    mark_it("parses a property correctly", [&] {
        auto elevation = object.get_numeric_property<int>("elevation");
        return test_that(elevation.value_or(0) == 10);
    });
});

describe<MapObject>("MapObject and its properties")([] {
    auto node = DocumentOwningNode::load_root(k_object_with_properties);
    assert(node);
    auto collection = MapObjectCollection::load_from(*node);
    auto * object = collection.seek_object_by_id(1);
    assert(object);
    mark_it("read string attribute", [&] {
        auto strattr = object->get_string_attribute("someattribute");
        if (strattr) {
            return test_that(::strcmp(strattr, "hello") == 0);
        }
        return test_that(false);
    }).
    mark_it("read numeric attribute", [&] {
        auto num = object->get_numeric_attribute<Real>("x");
        if (!num) return test_that(false);
        return test_that(are_very_close(*num, 426.604));
    }).
    mark_it("read object property", [&] {
        auto other_object = object->get_object_property("object");
        if (!other_object) return test_that(false);
        return test_that(other_object->id() == 3);
    }).
    mark_it("read group property", [&] {
        auto group = object->get_group_property("group");
        if (!group) return test_that(false);
        return test_that(group->id() == 2);
    }).
    mark_it("read numeric property", [&] {
        auto num = object->get_numeric_property<int>("numeric");
        if (!num) return test_that(false);
        return test_that(*num == 10);
    }).
    mark_it("read string property", [&] {
        auto str = object->get_string_property("string");
        if (!str) return test_that(false);
        return test_that(::strcmp(str, "hello mario") == 0);
    });
});

describe<MapObject>("MapObject::find_first_visible_named_objects")([] {
    auto node = DocumentOwningNode::load_root(k_object_up_tree_example);
    using GroupConstIterator = MapObjectGroup::ConstIterator;
    auto containers = MapObjectGroup::initialize_for_map(*node);
    auto & groups = std::get<GroupContainer>(containers);
    const auto & elements = std::get<std::vector<const TiXmlElement *>>(containers);
    auto objects = MapObject::load_objects_from
        (View<GroupConstIterator>{groups.begin(), groups.end()},
         View{elements.begin(), elements.end()});
    auto global_names = MapObject::find_first_visible_named_objects(objects);
    mark_it("there are exactly two names visible", [&] {
        return test_that(global_names.size() == 2);
    }).
    mark_it("an \"x\" is visible", [&] {
        auto itr = global_names.find("x");
        return test_that(itr != global_names.end());
    }).
    mark_it("it is the top most \"x\"", [&] {
        auto itr = global_names.find("x");
        if (itr == global_names.end())
            return test_that(false);
        return test_that(itr->second->id() == 1);
    }).
    mark_it("an \"something\" is visible", [&] {
        return test_that(global_names.find("something") !=
                         global_names.end()               );
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
