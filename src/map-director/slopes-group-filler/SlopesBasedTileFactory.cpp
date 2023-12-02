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

#include "SlopesBasedTileFactory.hpp"
#include "../TilesetPropertiesGrid.hpp"

#include <cstring>

SlopeGroupNeighborhood::SlopeGroupNeighborhood
    (const SlopesGridInterface & slopesintf, Vector2I tilelocmap):
    m_grid(&slopesintf),
    m_loc(tilelocmap) {}

Real SlopeGroupNeighborhood::neighbor_elevation(CardinalDirection dir) const {
    using Cd = CardinalDirection;

    using VecCdTup = Tuple<Vector2I, Cd>;
    static constexpr int k_neighbor_count = 3;
    auto select_el = [this] (const std::array<VecCdTup, k_neighbor_count> & arr) {
        // I miss map, transform sucks :c
        std::array<Real, k_neighbor_count> vals;
        std::transform(arr.begin(), arr.end(), vals.begin(), [this] (const VecCdTup & tup) {
            auto [r, d] = tup; {}
            return neighbor_elevation(r, d);
        });
        auto itr = std::find_if(vals.begin(), vals.end(),
            [] (Real x) { return cul::is_real(x); });
        return itr == vals.end() ? k_inf : *itr;
    };

    switch (dir) {
    case Cd::n: case Cd::s: case Cd::e: case Cd::w:
        throw InvalidArgument{"Not a corner"};
    case Cd::nw:
        return select_el(std::array{
            make_tuple(Vector2I{ 0, -1}, Cd::sw),
            make_tuple(Vector2I{-1 , 0}, Cd::ne),
            make_tuple(Vector2I{-1 ,-1}, Cd::se)
        });
    case Cd::sw:
        return select_el(std::array{
            make_tuple(Vector2I{-1,  0}, Cd::se),
            make_tuple(Vector2I{ 0,  1}, Cd::nw),
            make_tuple(Vector2I{-1 , 1}, Cd::ne)
        });
    case Cd::se:
        return select_el(std::array{
            make_tuple(Vector2I{ 1, 0}, Cd::sw),
            make_tuple(Vector2I{ 0, 1}, Cd::ne),
            make_tuple(Vector2I{ 1, 1}, Cd::nw)
        });
    case Cd::ne:
        return select_el(std::array{
            make_tuple(Vector2I{ 1,  0}, Cd::nw),
            make_tuple(Vector2I{ 0, -1}, Cd::se),
            make_tuple(Vector2I{ 1, -1}, Cd::sw)
        });
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

/* private */ Real SlopeGroupNeighborhood::neighbor_elevation
    (const Vector2I & r, CardinalDirection dir) const
{
    using Cd = CardinalDirection;
    auto get_slopes = [this, r] { return (*m_grid)(m_loc + r); };
    switch (dir) {
    case Cd::n: case Cd::s: case Cd::e: case Cd::w:
        throw InvalidArgument{"Not a corner"};
    case Cd::nw: return get_slopes().nw;
    case Cd::sw: return get_slopes().sw;
    case Cd::se: return get_slopes().se;
    case Cd::ne: return get_slopes().ne;
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

// ----------------------------------------------------------------------------

/* static */ const SlopeFillerExtra::SpecialTypeFuncMap &
    SlopeFillerExtra::special_type_funcs()
{
    static auto s_map = [] {
        SpecialTypeFuncMap map;
        map["pure-texture"] = &SlopeFillerExtra::setup_pure_texture;
        return map;
    } ();
    return s_map;
}

void SlopeFillerExtra::setup_pure_texture
    (const TilesetXmlGrid & xml_grid, const Vector2I & r)
{
    using cul::convert_to;
    Size2 scale{xml_grid.tile_size().width  / xml_grid.texture_size().width,
                xml_grid.tile_size().height / xml_grid.texture_size().height};
    Vector2 pos{r.x*scale.width, r.y*scale.height};
    TileTexture texture{pos, pos + convert_to<Vector2>(scale)};
    xml_grid(r).for_value("assignment", [this, texture](const auto & value) {
        m_pure_textures[value] = texture;
    });
}

// ----------------------------------------------------------------------------

void SlopesBasedTileFactory::setup
    (const TilesetXmlGrid & xml_grid,
     PlatformAssetsStrategy & platform,
     const SlopeFillerExtra & slope_extras,
     const Vector2I & location_on_tileset)
{
    TileFactory::setup(xml_grid, platform, location_on_tileset);
    setup_
        (xml_grid(location_on_tileset), platform, slope_extras,
         location_on_tileset);
}

// ----------------------------------------------------------------------------

CardinalDirection cardinal_direction_from(const std::string & str)
    { return cardinal_direction_from(str.c_str()); }

CardinalDirection cardinal_direction_from(const std::string * str) {
    return cardinal_direction_from(str ? str->c_str() : nullptr);
}

CardinalDirection cardinal_direction_from(const char * str) {
    auto seq = [str](const char * s) { return !::strcmp(str, s); };
    using Cd = CardinalDirection;
    if (!str) {
        throw InvalidArgument
            {"cardinal_direction_from: cannot convert nullptr to a cardinal "
             "direction"                                                     };
    }
    if (seq("n" )) return Cd::n;
    if (seq("s" )) return Cd::s;
    if (seq("e" )) return Cd::e;
    if (seq("w" )) return Cd::w;
    if (seq("ne")) return Cd::ne;
    if (seq("nw")) return Cd::nw;
    if (seq("se")) return Cd::se;
    if (seq("sw")) return Cd::sw;
    throw InvalidArgument
        {"cardinal_direction_from: cannot convert \"" + std::string{str} +
         "\" to a cardinal direction"};
}

