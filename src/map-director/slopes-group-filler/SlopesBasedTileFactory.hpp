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

#include "../TileFactory.hpp"

class ProducableTileCallbacks;

enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

class SlopesGridInterface {
public:
    virtual ~SlopesGridInterface() {}

    virtual Slopes operator () (Vector2I) const = 0;

    static const SlopesGridInterface & null_instance() {
        class Impl final : public SlopesGridInterface {
        public:
            Slopes operator () (Vector2I) const final
                { return Slopes{k_inf, k_inf, k_inf, k_inf}; }
        };
        static Impl impl;
        return impl;
    }
};

// there's a type of group'
// each group type is close to each type of tile factory
// therefore a tile factory knows how to create its own group
// TileFactory: virtual NeighborGroup * create_group() const = 0;
//
// do layers have positioning/information that accompany them that affects the
// production of tiles?
// possibly
// is a tile factory local to a tileset? local to a tileset & layer?

struct TileLocation;
class TileGroup;

/// describes neighbors and an address for a tile
class SlopeGroupNeighborhood final {
public:
    SlopeGroupNeighborhood
        (const SlopesGridInterface &, Vector2I tile_loc_on_map,
         Vector2I maps_spawner_offset);

    Real neighbor_elevation(CardinalDirection) const;

    Vector2I tile_location_on_field() const { return m_loc + m_offset; }

private:
    Real neighbor_elevation(const Vector2I &, CardinalDirection) const;

    const SlopesGridInterface * m_grid = &SlopesGridInterface::null_instance();
    Vector2I m_loc;
    Vector2I m_offset;
};

class TranslatableTileFactory : public TileFactory {
protected:
    Vector translation() const { return m_translation; }
#   if 0
    Entity make_entity
        (Platform & platform, Vector2I tile_loc,
         const SharedPtr<const RenderModel> & model_ptr) const;
#   endif
    void setup_
        (const Vector2I & loc_in_ts, const TileProperties &, Platform &) override;

    ModelTranslation translation_from_tile_location(const Vector2I & tile_loc) const
        { return ModelTranslation{m_translation + grid_position_to_v3(tile_loc)}; }

private:
    Vector m_translation;
};

/// Extra information for slope tiles maybe indicated.
/// Limits: does not describe relations between tiles
///         (that is what groups are for anyhow)
class SlopeFillerExtra final {
public:
    using TileTextureMap = std::map<std::string, TileTexture>;
    using SpecialTypeFunc = void(SlopeFillerExtra::*)(const TileSetXmlGrid & xml_grid, const Vector2I & r);
    using SpecialTypeFuncMap = std::map<std::string, SpecialTypeFunc>;

    static const SpecialTypeFuncMap & special_type_funcs();

    template <typename Key, typename Func>
    void for_texture(const Key & key, Func && f) const;

    void setup_pure_texture
        (const TileSetXmlGrid & xml_grid, const Vector2I & r);

    template <typename Key>
    void action_by_tile_type
        (const Key & key, const TileSetXmlGrid & xml_grid, const Vector2I & r,
         const SpecialTypeFuncMap & special_funcs_ = special_type_funcs());

private:
    TileTextureMap m_pure_textures;
};

template <typename Key>
void SlopeFillerExtra::action_by_tile_type
    (const Key & key, const TileSetXmlGrid & xml_grid, const Vector2I & r,
     const SpecialTypeFuncMap & special_funcs_)
{
    auto jtr = special_funcs_.find(key);
    if (jtr != special_funcs_.end()) {
        (this->*jtr->second)(xml_grid, r);
    }
}

template <typename Key, typename Func>
void SlopeFillerExtra::for_texture(const Key & key, Func && f) const {
    auto itr = m_pure_textures.find(key);
    if (itr == m_pure_textures.end())
        return;
    auto cpy = itr->second;
    f(cpy);
}

class SlopesBasedTileFactory : public TranslatableTileFactory {
public:
    virtual void operator ()
        (const SlopeGroupNeighborhood &,
         ProducableTileCallbacks &) const = 0;

    void setup(const TileSetXmlGrid &, Platform &,
               const SlopeFillerExtra &,
               const Vector2I & location_on_tileset);

    virtual Slopes tile_elevations() const = 0;

protected:
    virtual void setup_(const TileProperties &, Platform &,
                        const SlopeFillerExtra &,
                        const Vector2I & location_on_tileset) = 0;

    void setup_(const Vector2I &, const TileProperties &, Platform &) final {}
};

CardinalDirection cardinal_direction_from(const std::string & str);

CardinalDirection cardinal_direction_from(const std::string * str);

CardinalDirection cardinal_direction_from(const char * str);
