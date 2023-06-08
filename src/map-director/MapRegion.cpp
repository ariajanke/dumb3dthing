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

#include "MapRegion.hpp"
#include "TileFactory.hpp"
#include "MapRegionTracker.hpp"
#include "MapDirector.hpp"
#include "RegionLoadRequest.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;

Vector2I region_load_step
    (const ProducableTileViewGrid &, const RegionLoadRequest &);

} // end of <anonymous> namespace

TiledMapRegionN::TiledMapRegionN(ProducableTileViewGrid && full_factory_grid):
    m_producables_view_grid(std::move(full_factory_grid)) {}

void TiledMapRegionN::process_load_request
    (const RegionLoadRequest & request, const Vector2I & offset,
     RegionLoadCollectorN & collector)
{
    // reminder: tiles are laid out eastward (not westward)
    //           it's assumed that bottom-top interpretation of a tiled map is
    //           not comfortable (and therefore top-down)
    auto step = region_load_step(m_producables_view_grid, request);
    auto subgrid_size = cul::convert_to<Size2I>(step);
    for (Vector2I r; r.x < m_producables_view_grid.width (); r.x += step.x) {
    for (r.y = 0   ; r.y < m_producables_view_grid.height(); r.y += step.y) {
        auto on_field_position = offset + r;
        bool overlaps_this_subregion =
            request.overlaps_with(RectangleI{on_field_position, subgrid_size});
        if (!overlaps_this_subregion) continue;
        auto subgrid = m_producables_view_grid.make_subgrid(RectangleI{r, subgrid_size});
        collector.add_tiles(on_field_position, offset, subgrid);
    }}
}

namespace {

Vector2I region_load_step
    (const ProducableTileViewGrid & view_grid,
     const RegionLoadRequest & request)
{
    auto step_of_ = [] (int length, int max) {
        // I want as even splits as possible
        if (length < max) return length;
        return length / (length / max);
    };
    return Vector2I
        {step_of_(view_grid.width (), request.max_region_size().width ),
         step_of_(view_grid.height(), request.max_region_size().height)};
}

} // end of <anonymous> namespace
