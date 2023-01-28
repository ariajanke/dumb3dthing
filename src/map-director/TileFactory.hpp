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
#include "ParseHelpers.hpp"
#include "TileTexture.hpp"

#include "../Defs.hpp"
#include "../platform.hpp"

#include <map>

class TileProperties;

class EntityAndTrianglesAdder {
public:
    virtual ~EntityAndTrianglesAdder() {}

    virtual void add_entity(const Entity &) = 0;

    virtual void add_triangle(const TriangleSegment &) = 0;
};

class TileSetXmlGrid;

/// A tile factory is a thing that produces tiles.
/// It is local to the tileset
class TileFactory {
public:
    virtual ~TileFactory() {}

    void setup(const TileSetXmlGrid &, Platform &,
               const Vector2I & location_on_tileset);

protected:
    static void add_triangles_based_on_model_details
        (Vector2I gridloc, Vector translation, const Slopes & slopes,
         EntityAndTrianglesAdder & adder);

    static Vector grid_position_to_v3(const Vector2I & r)
        { return Vector{r.x, 0, -r.y}; }

    static std::array<Vector, 4> get_points_for(const Slopes &);

    static const std::vector<unsigned> & get_common_elements();

    SharedPtr<const Texture> common_texture() const
        { return m_texture_ptr; }

    std::array<Vector2, 4> common_texture_positions_from(Vector2I ts_r) const;

    Entity make_entity(Platform & platform, Vector translation,
                       const SharedPtr<const RenderModel> & model_ptr) const;

    SharedPtr<const RenderModel> make_render_model_with_common_texture_positions
        (Platform & platform, const Slopes & slopes, Vector2I loc_in_ts) const;

    Size2 common_texture_tile_size() const;

    Vector2 common_texture_origin(Vector2I ts_r) const;

    TileTexture floor_texture_at(Vector2I) const;

    virtual void setup_
        (const Vector2I & loc_in_ts, const TileProperties & properties, Platform &) = 0;

private:
    void set_shared_texture_information
        (const SharedPtr<const Texture> & texture_ptr_, const Size2 & texture_size_,
         const Size2 & tile_size_);

    SharedPtr<const Texture> m_texture_ptr;
    Size2 m_texture_size;
    Size2 m_tile_size;
};