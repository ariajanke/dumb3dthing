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

#include "MapRegionContainer.hpp"

namespace {

using RegionRefresh = MapRegionContainer::RegionRefresh;

} // end of <anonymous> namespace

Optional<RegionRefresh> MapRegionContainer::region_refresh_at
    (const Vector2I & on_field_position)
{
    auto itr = m_loaded_regions.find(on_field_position);
    if (itr == m_loaded_regions.end()) return {};
    return RegionRefresh{itr->second.keep_on_refresh};
}

void MapRegionContainer::decay_regions(RegionDecayAdder & decay_adder) {
    for (auto itr = m_loaded_regions.begin(); itr != m_loaded_regions.end(); ) {
        if (itr->second.keep_on_refresh) {
            itr->second.keep_on_refresh = false;
            ++itr;
        } else {
            decay_adder.add(itr->first,
                            std::move(itr->second.triangle_grid),
                            std::move(itr->second.entities));
            itr = m_loaded_regions.erase(itr);
        }
    }
}

void MapRegionContainer::set_region
    (const Vector2I & on_field_position,
     const ScaledTriangleViewGrid & triangle_grid,
     std::vector<Entity> && entities)
{
    auto * region = &m_loaded_regions[on_field_position];
    region->entities = std::move(entities);
    region->triangle_grid = triangle_grid;
    region->keep_on_refresh = true;
}
