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

/* static */ SharedPtr<TilesetBase> TilesetBase::make
    (const TiXmlElement & tileset_el)
{
    const char * type = nullptr;
    auto * properties = tileset_el.FirstChildElement("properties");
    if (properties) {
        for (auto & property : XmlRange{properties, "property"}) {
            type = property.Attribute("type");
        }
    }

    SharedPtr<TilesetBase> rv;
    if (!type) {
        return make_shared<ProducablesTileset>();
    } else if (::strcmp(type, "composite-tile-set")) {
        return make_shared<CompositeTileset>();
    }
    return nullptr;
}

Vector2I TilesetBase::tile_id_location(int tid) const {
    auto sz = size2();
    if (tid < 0 || tid >= sz.height*sz.height) {
        throw std::invalid_argument{""};
    }
    return Vector2I{tid % sz.width, tid / sz.width};
}

int TilesetBase::total_tile_count() const {
    auto sz = size2();
    return sz.width*sz.height;
}
