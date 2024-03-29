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

#include "map-loader.hpp"
#include "TileTexture.hpp"
#include "ProducableGrid.hpp"

#include "../Definitions.hpp"
#include "../platform.hpp"

class TileProperties;
class ProducableTileCallbacks;
class TilesetXmlGrid;

/// A tile factory is a thing that produces tiles.
/// It is local to the tileset
/// Can be used as a shared utility by producable tiles.
class TileFactory {
public:
    virtual ~TileFactory() {}

    void setup(const TilesetXmlGrid &,
               PlatformAssetsStrategy &,
               const Vector2I & location_on_tileset);
protected:
    static void add_triangles_based_on_model_details
        (const Vector & translation, const Slopes & slopes,
         ProducableTileCallbacks & callbacks);

    static std::array<Vector, 4> get_points_for(const Slopes &);

    static const std::vector<unsigned> & get_common_elements();

    SharedPtr<const Texture> common_texture() const
        { return m_texture_ptr; }

    std::array<Vector2, 4> common_texture_positions_from(Vector2I ts_r) const;

    template <typename Head, typename ... Types>
    Entity add_visual_entity_with(
        ProducableTileCallbacks & callbacks, Head && head, Types &&... arguments) const;

    SharedPtr<const RenderModel> make_render_model_with_common_texture_positions
        (PlatformAssetsStrategy & platform,
         const Slopes & slopes,
         Vector2I location_in_tileset) const;

    Size2 common_texture_tile_size() const;

    Vector2 common_texture_origin(Vector2I ts_r) const;

    TileTexture floor_texture_at(Vector2I) const;

    virtual void setup_
        (const Vector2I & location_in_tileset,
         const TileProperties & properties,
         PlatformAssetsStrategy &) = 0;

private:
    void set_shared_texture_information
        (const SharedPtr<const Texture> & texture_ptr_, const Size2 & texture_size_,
         const Size2 & tile_size_);

    SharedPtr<const Texture> m_texture_ptr;
    Size2 m_texture_size;
    Size2 m_tile_size;
};

template <typename Head, typename ... Types>
Entity TileFactory::add_visual_entity_with(
    ProducableTileCallbacks & callbacks, Head && head, Types &&... arguments) const
{
    static_assert
        (TypeList<Head, Types...>::
         template RemoveIf<std::is_reference>::
         template kt_equal_to_list<Head, Types...>,
        "No reference types allowed");
    return callbacks.add_entity
        <SharedPtr<const Texture>, ModelVisibility, Head, Types...>
        (common_texture(), ModelVisibility{true}, std::move(head), std::forward<Types>(arguments)...);
}
