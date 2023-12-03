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

#include "MapLoadingError.hpp"

#include "../ParseHelpers.hpp"
#include "../ProducableGroupFiller.hpp"
#include "../CompositeMapRegion.hpp"

#include <map>

class TilesetXmlGrid;
class StackableSubRegionGrid;
class TilesetMappingTile;
class TilesetLayerWrapper;
class MapContentLoader;
class StackableProducableTileGrid;

class TilesetMapElementCollector {
public:
    using ProducableOwnerCollection =
        ProducableTileGridStacker::ProducableOwnerCollection;

    virtual ~TilesetMapElementCollector() {}

    virtual void add
        (Grid<ProducableTile *> && producables,
         ProducableOwnerCollection && producable_owners) = 0;

    virtual void add
        (Grid<const MapSubRegion *> && subregions,
         const SharedPtr<Grid<MapSubRegion>> & owner) = 0;
};

class MapContentLoader : public PlatformAssetsStrategy {
public:
    using FillerFactory =
        SharedPtr<ProducableGroupFiller>(*)
        (const TilesetXmlGrid &, PlatformAssetsStrategy &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;
    using TaskContinuation = BackgroundTask::Continuation;

    static const FillerFactoryMap & builtin_fillers();

    virtual ~MapContentLoader() {}

    virtual const FillerFactoryMap & map_fillers() const = 0;

    /// @returns true if any promised file contents is not immediately ready
    virtual bool delay_required() const = 0;

    virtual void add_warning(MapLoadingWarningEnum) = 0;

    virtual void wait_on(const SharedPtr<BackgroundTask> &) = 0;

    virtual TaskContinuation & task_continuation() const = 0;
};

class TilesetBase {
public:
    using MappingContainer = std::vector<TilesetMappingTile>;
    using MappingView = View<MappingContainer::const_iterator>;
    using Continuation = BackgroundTask::Continuation;
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;
    using FillerFactory = MapContentLoader::FillerFactory;
    using FillerFactoryMap = MapContentLoader::FillerFactoryMap;

    static SharedPtr<TilesetBase> make(const TiXmlElement &);

    virtual ~TilesetBase() {}

    [[nodiscard]] virtual Continuation & load
        (const TiXmlElement &, MapContentLoader &) = 0;

    virtual void add_map_elements
        (TilesetMapElementCollector &, const TilesetLayerWrapper & mapping_view) const = 0;

    Vector2I tile_id_location(int tile_id) const;

    int total_tile_count() const;

protected:
    virtual Size2I size2() const = 0;
};
