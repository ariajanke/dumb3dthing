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

namespace {

using Continuation = BackgroundTask::Continuation;

class CompositeTilesetFinisherTask final : public BackgroundTask {
public:
    static RectangleI get_sub_rectangle_of
        (const Vector2I & position,
         const Grid<MapSubRegion> & sub_region_grid,
         const MapRegion & map_region);

    CompositeTilesetFinisherTask
        (const SharedPtr<MapLoaderTask> & map_loader_task_,
         SharedPtr<Grid<MapSubRegion>> & sub_regions_grid_,
         SharedPtr<MapRegion> & source_map_);

    Continuation & in_background
        (Callbacks &, ContinuationStrategy &) final;

private:
    static SharedPtr<Grid<MapSubRegion>> &
        verify_sub_regions_grid(SharedPtr<Grid<MapSubRegion>> &);

    static const SharedPtr<MapLoaderTask> &
        verify_map_loader_task(const SharedPtr<MapLoaderTask> &);

    static SharedPtr<MapRegion> & verify_source_map(SharedPtr<MapRegion> &);

    SharedPtr<BackgroundTask> m_map_loader_task;
    SharedPtr<MapLoaderTask> m_map_retriever;
    SharedPtr<Grid<MapSubRegion>> & m_sub_regions_grid;
    SharedPtr<MapRegion> & m_source_map;
};

} // end of <anonymous> namespace

/* static */ Grid<const MapSubRegion *> CompositeTileset::to_layer
    (const Grid<MapSubRegion> & sub_regions_grid,
     const TilesetLayerWrapper & layer_wrapper)
{
    Grid<const MapSubRegion *> sub_region_layer;
    sub_region_layer.set_size(layer_wrapper.grid_size(), nullptr);
    for (auto & location : layer_wrapper) {
        sub_region_layer(location.on_map()) =
            &sub_regions_grid(location.on_tile_set());
    }
    return sub_region_layer;
}

/* static */ Optional<Size2I> CompositeTileset::size_of_tileset
    (const TiXmlElement & tileset_element)
{
    static constexpr const int k_no_number = -1;
    auto count = tileset_element.IntAttribute("tilecount", k_no_number);
    auto columns = tileset_element.IntAttribute("columns", k_no_number);
    if (count == k_no_number || columns == k_no_number ||
        count <= 0           || count % columns != 0     )
        { return {}; }
    return Size2I{columns, count / columns};
}

Continuation & CompositeTileset::load
    (const DocumentOwningXmlElement & tileset_element, MapContentLoader & content_loader)
{
    SharedPtr<MapLoaderTask> map_loader_task;
    auto properties = tileset_element->FirstChildElement("properties");
    for (auto & property : XmlRange{properties, "property"}) {
        auto name = property.Attribute("name");
        auto value = property.Attribute("value");
        if (!name || !value) continue;
        if (::strcmp(name, "filename") == 0) {
            map_loader_task = make_shared<MapLoaderTask>(value, content_loader);
        }
    }
    if (!map_loader_task) {
        throw RuntimeError{"Unhandled: composite tileset without a filename"};
    }
    m_sub_regions_grid = make_shared<Grid<MapSubRegion>>();
    m_sub_regions_grid->set_size
        (*size_of_tileset(*tileset_element), MapSubRegion{});
    auto task_to_wait_on = make_shared<CompositeTilesetFinisherTask>
        (std::move(map_loader_task), m_sub_regions_grid, m_source_map);
    content_loader.wait_on(task_to_wait_on);
    return content_loader.task_continuation();
}

void CompositeTileset::add_map_elements
    (TilesetMapElementCollector & collector,
     const TilesetLayerWrapper & layer_wrapper) const
{
    collector.add
        (to_layer(*m_sub_regions_grid, layer_wrapper),
         m_sub_regions_grid);
}

namespace {

/* static */ RectangleI CompositeTilesetFinisherTask::get_sub_rectangle_of
    (const Vector2I & position,
     const Grid<MapSubRegion> & sub_region_grid,
     const MapRegion & map_region)
{
    auto map_region_size = map_region.size2();
    auto width  = map_region_size.width  / sub_region_grid.width ();
    auto height = map_region_size.height / sub_region_grid.height();
    return RectangleI{position.x*width, position.y*height, width, height};
}

CompositeTilesetFinisherTask::CompositeTilesetFinisherTask
    (const SharedPtr<MapLoaderTask> & map_loader_task_,
     SharedPtr<Grid<MapSubRegion>> & sub_regions_grid_,
     SharedPtr<MapRegion> & source_map_):
    m_map_loader_task(verify_map_loader_task(map_loader_task_)),
    m_map_retriever(map_loader_task_),
    m_sub_regions_grid(verify_sub_regions_grid(sub_regions_grid_)),
    m_source_map(verify_source_map(source_map_)) {}

Continuation & CompositeTilesetFinisherTask::in_background
    (Callbacks &, ContinuationStrategy & strategy)
{
    if (m_map_loader_task) {
        auto ml = std::move(m_map_loader_task);
        return strategy.continue_().wait_on(ml);
    }

    auto res = m_map_retriever->retrieve();
    m_source_map = std::move(res.map_region);
    for (Vector2I r;
         r != m_sub_regions_grid->end_position();
         r = m_sub_regions_grid->next(r))
    {
        auto subrect = get_sub_rectangle_of
            (r, *m_sub_regions_grid, *m_source_map);
        (*m_sub_regions_grid)(r) = MapSubRegion{subrect, m_source_map};
    }
    return strategy.finish_task();
}

/* private static */ SharedPtr<Grid<MapSubRegion>> &
    CompositeTilesetFinisherTask::verify_sub_regions_grid
    (SharedPtr<Grid<MapSubRegion>> & sub_regions_grid)
{
    if (!sub_regions_grid) {
        throw InvalidArgument
            {"Forgot to instantiate a owner grid"};
    }
    if (sub_regions_grid->size2() == Size2I{}) {
        throw InvalidArgument
            {"Forgot to set a size for owner grid"};
    }
    return sub_regions_grid;
}

/* private static */ const SharedPtr<MapLoaderTask> &
    CompositeTilesetFinisherTask::verify_map_loader_task
    (const SharedPtr<MapLoaderTask> & map_loader_task)
{
    if (map_loader_task) return map_loader_task;
    throw InvalidArgument{"Forgot to instantiate map loader task"};
}

/* private static */ SharedPtr<MapRegion> &
    CompositeTilesetFinisherTask::verify_source_map
    (SharedPtr<MapRegion> & source_map)
{
    if (!source_map) return source_map;
    throw InvalidArgument{"Forgot to not instantiate source map"};
}

} // end of <anonymous> namespace
