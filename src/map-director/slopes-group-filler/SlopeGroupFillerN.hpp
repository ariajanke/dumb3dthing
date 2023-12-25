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

#include "../ProducableGroupFiller.hpp"
#include "../TilesetPropertiesGrid.hpp"

// a tileset tile being a factory is not a terrible idea
// however the current implementation is terrible
#if 0
enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

class TileCornerElevations final {
public:
    TileCornerElevations() {}

    TileCornerElevations
        (Real ne_, Real nw_, Real sw_, Real se_):
        m_nw(nw_), m_ne(ne_), m_sw(sw_), m_se(se_) {}

    bool operator == (const TileCornerElevations & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const TileCornerElevations & rhs) const noexcept
        { return !are_same(rhs); }

    Real north_east() const { return m_ne; }

    Real north_west() const { return m_nw; }

    Real south_east() const { return m_se; }

    Real south_west() const { return m_sw; }

private:
    bool are_same(const TileCornerElevations & rhs) const noexcept {
        using Fe = std::equal_to<Real>;
        return    Fe{}(m_nw, rhs.m_nw) && Fe{}(m_ne, rhs.m_ne)
               && Fe{}(m_sw, rhs.m_sw) && Fe{}(m_se, rhs.m_se);
    }

    Real m_nw, m_ne, m_sw, m_se;
};
#if 0
class TileCornerElevationsGridInterface {
public:
    virtual ~TileCornerElevationsGridInterface() {}

    virtual TileCornerElevations operator () (const Vector2I &) const = 0;
};
#endif
// this kind of factory originates from the tileset and knows it's neighbors?
class SlopesTilesetTile {
public:
    static SharedPtr<SlopesTilesetTile> make();

    virtual ~SlopesTilesetTile() {}

    virtual const TileCornerElevations & corner_elevations() const = 0;

    // can add the optimization that producables are created from a vector
    virtual SharedPtr<ProducableTile> make_producable
        (const TileCornerElevations & neighboring_elevations,
         const Vector2I & position_in_region) const = 0;
};
#endif
#if 0
class SlopeGroupFiller final : public ProducableGroupFiller {
public:
    using RampGroupFactoryMakeFunc = UniquePtr<SlopesBasedTileFactory>(*)();
    using RampGroupFactoryMap = std::map<std::string, RampGroupFactoryMakeFunc>;
    using TileFactoryGrid = Grid<SharedPtr<SlopesBasedTileFactory>>;

    void make_group(CallbackWithCreator &) const final;

    void load
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform,
         const RampGroupFactoryMap & factory_type_map = builtin_tile_factory_maker_map());

    static const RampGroupFactoryMap & builtin_tile_factory_maker_map();

private:
    void load_factories(const TilesetXmlGrid & xml_grid,
                        const RampGroupFactoryMap & factory_type_map);

    void setup_factories
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform,
         TileFactoryGrid &) const;

    SlopeFillerExtra m_specials;
    TileFactoryGrid m_tile_factories;
};
#endif
class SlopesTilesetTile;

class SlopeGroupFiller final : public ProducableGroupFiller {
public:
    using TilesetTilePtr = SharedPtr<SlopesTilesetTile>;
    using TilesetTileMakerFunction = TilesetTilePtr(*)();
    using TilesetTileMakerMap = std::map<std::string, TilesetTileMakerFunction>;
    using TilesetTileGridPtr = SharedPtr<Grid<TilesetTilePtr>>;

    static const TilesetTileMakerMap & builtin_makers();

    void make_group(CallbackWithCreator &) const final;

    void load
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform,
         const TilesetTileMakerMap & = builtin_makers());

private:
    void load_factories(const TilesetXmlGrid & xml_grid);

    void setup_factories
        (const TilesetXmlGrid & xml_grid,
         PlatformAssetsStrategy & platform) const;

    TilesetTileGridPtr m_tileset_tiles;
};
