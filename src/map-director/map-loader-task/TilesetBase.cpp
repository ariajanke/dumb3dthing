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

#include "TilesetBase.hpp"
#include "ProducablesTileset.hpp"
#include "CompositeTileset.hpp"

#include <tinyxml2.h>

namespace {

using FillerFactoryMap = TilesetBase::FillerFactoryMap;

} // end of <anonymous> namespace

/* static */ const FillerFactoryMap & MapContentLoader::builtin_fillers()
    { return ProducablesTileset::builtin_fillers(); }

/* static */ SharedPtr<TilesetBase> TilesetBase::make
    (const TiXmlElement & tileset_el)
{
    const char * type = nullptr;
    auto * properties = tileset_el.FirstChildElement("properties");
    if (properties) {
        for (auto & property : XmlRange{properties, "property"}) {
            auto name = property.Attribute("name");
            auto value = property.Attribute("value");
            if (::strcmp(name, "type") == 0 && value)
                type = value;
        }
    }

    SharedPtr<TilesetBase> rv;
    if (!type) {
        return make_shared<ProducablesTileset>();
    } else if (::strcmp(type, "composite-map-tileset") == 0) {
        return make_shared<CompositeTileset>();
    }
    return nullptr;
}

Vector2I TilesetBase::tile_id_location(int tid) const {
    auto sz = size2();
    if (tid < 0 || tid >= sz.height*sz.height) {
        throw InvalidArgument{"tile id is not valid"};
    }
    return Vector2I{tid % sz.width, tid / sz.width};
}

int TilesetBase::total_tile_count() const {
    auto sz = size2();
    return sz.width*sz.height;
}
