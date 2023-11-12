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

#pragma once

#include "TilesetBase.hpp"

class CompositeTileset final : public TilesetBase {
public:
    // loading this: will have to go through steps of promises and so on
    // in order to load, so it needs to be non-blocking

    BackgroundTaskCompletion load(Platform &, const TiXmlElement &) final;

    void add_map_elements(TilesetMapElementVisitor &, const TilesetLayerWrapper &) const final;

private:
    Size2I size2() const final { return m_sub_regions_grid.size2(); }

    Grid<MapSubRegion> m_sub_regions_grid;
    const SharedPtr<MapRegion> & parent_region();
};
