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

#include <map>

class TileSetXmlGrid;

class StackableSubRegionGrid final {
public:
    explicit StackableSubRegionGrid(Grid<MapSubRegion> &&);
};

class TileSetIdGrid;

class TileSetMapElementVisitor {
public:
    virtual ~TileSetMapElementVisitor() {}

    virtual void add(StackableProducableTileGrid &&) = 0;

    virtual void add(StackableSubRegionGrid &&) = 0;
};

class TileSetMappingTile;
class TileSetLayerWrapper;

class TileSetBase {
public:
    using MappingContainer = std::vector<TileSetMappingTile>;
    using MappingView = View<MappingContainer::const_iterator>;

    static SharedPtr<TileSetBase> make_and_load_tileset
        (Platform &, const TiXmlElement &);

    virtual ~TileSetBase() {}

    virtual void load(Platform &, const TiXmlElement &) = 0;

    virtual void add_map_elements
        (TileSetMapElementVisitor &, const TileSetLayerWrapper & mapping_view) const = 0;

    Vector2I tile_id_location(int tid) const {
        auto sz = size2();
        if (tid < 0 || tid >= sz.height*sz.height) {
            throw std::invalid_argument{""};
        }
        return Vector2I{tid % sz.width, tid / sz.width};
    }

    int total_tile_count() const {
        auto sz = size2();
        return sz.width*sz.height;
    }

protected:
    virtual Size2I size2() const = 0;
};

/// Tilesets map tileset ids to tile group fillers.
///
/// maybe a loader thing
class TileSet final : public TileSetBase {
public:
    using FillerFactory = SharedPtr<ProducableGroupFiller>(*)(const TileSetXmlGrid &, Platform &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;
    using TileSetBase::load;

    static const FillerFactoryMap & builtin_fillers();

    void load(Platform &, const TiXmlElement &,
              const FillerFactoryMap &);

    void load(Platform & platform, const TiXmlElement & element) final
        { load(platform, element, builtin_fillers()); }

    void add_map_elements(TileSetMapElementVisitor &, const TileSetLayerWrapper & mapping_view) const final;

private:
    Size2I size2() const final { return m_filler_grid.size2(); }

    // TODO
    // kind of ugly in general, reveals the need for refactoring of filler
    // classes, BUT not doing yet
    std::map<
        SharedPtr<ProducableGroupFiller>,
        std::vector<TileLocation>>
        make_fillers_and_locations(const TileSetLayerWrapper &) const;

    Grid<SharedPtr<ProducableGroupFiller>> m_filler_grid;
    std::vector<SharedPtr<const ProducableGroupFiller>> m_unique_fillers;
};

class CompositeTileSet final : public TileSetBase {
public:
    // loading this: will have to go through steps of promises and so on
    // in order to load, so it needs to be non-blocking

    void load(Platform &, const TiXmlElement &) final;

    void add_map_elements(TileSetMapElementVisitor &, const TileSetLayerWrapper & mapping_view) const final;

private:
    Size2I size2() const final { return m_sub_regions_grid.size2(); }

    Grid<MapSubRegion> m_sub_regions_grid;
    const SharedPtr<MapRegion> & parent_region();
};
