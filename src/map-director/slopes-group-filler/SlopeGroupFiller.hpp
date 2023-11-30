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

#include "SlopesBasedTileFactory.hpp"

#include "../ProducableGroupFiller.hpp"
#include "../TilesetPropertiesGrid.hpp"

class ProducableSlopeTile final : public ProducableTile {
public:
    using TileFactoryGridPtr = SharedPtr<Grid<SlopesBasedTileFactory *>>;

    ProducableSlopeTile
        (const Vector2I & map_position,
         const TileFactoryGridPtr & factory_map_layer);

    void operator () (ProducableTileCallbacks & callbacks) const final;

private:
    Vector2I m_map_position;

    TileFactoryGridPtr m_factory_map_layer;
};

class SlopeGroupFiller final : public ProducableGroupFiller {
public:
    using RampGroupFactoryMakeFunc = UniquePtr<SlopesBasedTileFactory>(*)();
    using RampGroupFactoryMap = std::map<std::string, RampGroupFactoryMakeFunc>;

    static SharedPtr<Grid<SlopesBasedTileFactory *>> make_factory_grid_for_map
        (const std::vector<TileLocation> & tile_locations,
         const Grid<SharedPtr<SlopesBasedTileFactory>> & tile_factories);

    ProducableGroupTileLayer operator ()
        (const std::vector<TileLocation> & tile_locations,
         ProducableGroupTileLayer && group_grid) const final;

    void load
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform,
         const RampGroupFactoryMap & factory_type_map = builtin_tile_factory_maker_map());

    static const RampGroupFactoryMap & builtin_tile_factory_maker_map();

private:
    using TileFactoryGrid = Grid<SharedPtr<SlopesBasedTileFactory>>;

    void load_factories(const TilesetXmlGrid & xml_grid,
                        const RampGroupFactoryMap & factory_type_map);

    void setup_factories
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform,
         TileFactoryGrid &) const;

    SlopeFillerExtra m_specials;
    TileFactoryGrid m_tile_factories;
};
