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

#include "CompositeTileset.hpp"
#include "MapLoaderTask.hpp"

#include <tinyxml2.h>

BackgroundTaskCompletion CompositeTileset::load
    (Platform & platform, const TiXmlElement & tileset_element)
{
    SharedPtr<MapLoaderTask> map_loader_task;
    auto properties = tileset_element.FirstChildElement("properties");
    for (auto & property : XmlRange{properties, "property"}) {
        auto name = property.Attribute("name");
        auto value = property.Attribute("value");
        if (!name || !value) continue;
        if (::strcmp(name, "filename") == 0) {
            map_loader_task = make_shared<MapLoaderTask>(value, platform);
        }
    }
    map_loader_task->
        set_return_task(BackgroundTask::make
        ([this, map_loader_task] (TaskCallbacks &)
        {
            m_source_map = map_loader_task->retrieve();
            return BackgroundTaskCompletion::k_finished;
        }));
    return BackgroundTaskCompletion{map_loader_task};
}

void CompositeTileset::add_map_elements
    (TilesetMapElementCollector &,
     const TilesetLayerWrapper & layer_wrapper) const
{
    for (auto & location : layer_wrapper) {
        ;
    }
}
