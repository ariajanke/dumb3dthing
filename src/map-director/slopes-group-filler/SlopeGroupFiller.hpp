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

#include "../ProducableTileFiller.hpp"
#include "../TileSetPropertiesGrid.hpp"
#include "SlopesBasedTileFactory.hpp"

class ProducableSlopeTile final : public ProducableTile {
public:
    using TileFactoryGridPtr = SharedPtr<Grid<SlopesBasedTileFactory *>>;

    ProducableSlopeTile
        (const Vector2I & map_position,
         const TileFactoryGridPtr & factory_map_layer);

    void operator ()
        (const Vector2I & maps_offset, EntityAndTrianglesAdder & adder,
         Platform & platform) const final;

private:
    Vector2I m_map_position;

    TileFactoryGridPtr m_factory_map_layer;
};

class SlopeGroupFiller final : public ProducableTileFiller {
public:
    using RampGroupFactoryMakeFunc = UniquePtr<SlopesBasedTileFactory>(*)();
    using RampGroupFactoryMap = std::map<std::string, RampGroupFactoryMakeFunc>;
    using TileTextureMap = std::map<std::string, TileTexture>;
    using SpecialTypeFunc = void(SlopeGroupFiller::*)(const TileSetXmlGrid & xml_grid, const Vector2I & r);
    using SpecialTypeFuncMap = std::map<std::string, SpecialTypeFunc>;

    static SharedPtr<Grid<SlopesBasedTileFactory *>> make_factory_grid_for_map
        (const std::vector<TileLocation> & tile_locations,
         const Grid<SharedPtr<SlopesBasedTileFactory>> & tile_factories);

    UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> & tile_locations,
         UnfinishedTileGroupGrid && group_grid) const final;

    void load
        (const TileSetXmlGrid & xml_grid, Platform & platform,
         const RampGroupFactoryMap & factory_type_map = builtin_tile_factory_maker_map());

    // dangler hazard while moving shit around

    static const RampGroupFactoryMap & builtin_tile_factory_maker_map();

private:
    template <typename T>
    static UniquePtr<SlopesBasedTileFactory> make_unique_base_factory() {
        static_assert(std::is_base_of_v<SlopesBasedTileFactory, T>);
        return make_unique<T>();
    }

    void setup_pure_texture
        (const TileSetXmlGrid & xml_grid, const Vector2I & r);

    static const SpecialTypeFuncMap & special_type_funcs();

    TileTextureMap m_pure_textures;
    Grid<SharedPtr<SlopesBasedTileFactory>> m_tile_factories;
};
