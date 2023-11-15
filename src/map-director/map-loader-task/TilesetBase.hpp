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

#include "../ParseHelpers.hpp"
#include "../ProducableGroupFiller.hpp"
#include "../MapRegion.hpp"

class TilesetXmlGrid;
class StackableSubRegionGrid;
class TilesetMappingTile;
class TilesetLayerWrapper;
struct MapContentLoader;

class TilesetMapElementCollector {
public:
    virtual ~TilesetMapElementCollector() {}

    virtual void add(StackableProducableTileGrid &&) = 0;

    virtual void add(StackableSubRegionGrid &&) = 0;
};

class TilesetBase {
public:
    using MappingContainer = std::vector<TilesetMappingTile>;
    using MappingView = View<MappingContainer::const_iterator>;

    static SharedPtr<TilesetBase> make(const TiXmlElement &);

    virtual ~TilesetBase() {}

    // might not have something to wait on
    [[nodiscard]] virtual BackgroundTaskCompletion load(Platform &, const TiXmlElement &) = 0;

    virtual void add_map_elements
        (TilesetMapElementCollector &, const TilesetLayerWrapper & mapping_view) const = 0;

    Vector2I tile_id_location(int tile_id) const;

    int total_tile_count() const;

protected:
    virtual Size2I size2() const = 0;
};
