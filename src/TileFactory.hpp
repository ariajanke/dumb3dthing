/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "Defs.hpp"
#include "map-loader.hpp"
#include "ParseHelpers.hpp"

#include <map>

class TileSet;

class EntityAndTrianglesAdder {
public:
    virtual ~EntityAndTrianglesAdder() {}

    virtual void add_triangle(const TriangleSegment &) = 0;

    virtual void add_entity(const Entity &) = 0;
};

enum class CardinalDirection {
    n, s, e, w,
    nw, sw, se, ne
};

struct WallElevationAndDirection final {
    CardinalDirection direction;
    Vector2I tileset_location;
    std::array<Real, 4> dip_heights;
};

class WallElevationAndDirectionComparator final {
    static auto tileset_location_list(const Vector2I & v)
        { return std::array { v.x, v.y }; };

    template <typename T, std::size_t kt_size>
    static T difference_between
        (const std::array<T, kt_size> & lhs, const std::array<T, kt_size> & rhs)
    {
        for (int i = 0; i != int(lhs.size()); ++i) {
            auto diff = lhs[i] - rhs[i];
            if (!are_very_close(diff, 0)) // <- should be okay for both fp & int
                return diff;
        }
        return 0;
    }

public:
    using Wed = WallElevationAndDirection;
    bool operator () (const Wed & lhs, const Wed & rhs) const {
        auto diff = static_cast<int>(lhs.direction) - static_cast<int>(rhs.direction);
        if (diff) return diff < 0;

        auto slopes_diff = difference_between(lhs.dip_heights, rhs.dip_heights);
        if (!are_very_close(slopes_diff, 0)) return slopes_diff < 0;
        auto tileset_location_diff = difference_between(
            tileset_location_list(lhs.tileset_location),
            tileset_location_list(rhs.tileset_location));
        return tileset_location_diff < 0;
    }
};

using WallRenderModelCache = std::map<
    WallElevationAndDirection,
    Tuple<SharedPtr<const RenderModel>, std::vector<TriangleSegment>>,
    WallElevationAndDirectionComparator>;

class TileFactory {
public:
    class NeighborInfo final {
    public:
        NeighborInfo
            (const SharedPtr<const TileSet> & ts, const Grid<int> & layer,
             Vector2I tilelocmap, Vector2I spawner_offset);

        NeighborInfo
            (const TileSet & ts, const Grid<int> & layer,
             Vector2I tilelocmap, Vector2I spawner_offset);

        static NeighborInfo make_no_neighbor();

        Real neighbor_elevation(CardinalDirection) const;

        Vector2I tile_location() const { return m_loc + m_offset; }

        // bad name: is actually more local
        Vector2I tile_location_in_map() const { return m_loc; }

    private:
        Real neighbor_elevation(const Vector2I &, CardinalDirection) const;

        const TileSet & m_tileset;
        const Grid<int> & m_layer;
        Vector2I m_loc;
        Vector2I m_offset;
    };

    virtual ~TileFactory() {}

    virtual void operator ()
        (EntityAndTrianglesAdder &, const NeighborInfo &, Platform::ForLoaders &) const = 0;

    void set_shared_texture_information
        (const SharedCPtr<Texture> & texture_ptr_, const Size2 & texture_size_,
         const Size2 & tile_size_);

    virtual void setup(Vector2I loc_in_ts, const TiXmlElement * properties,
                       Platform::ForLoaders &) = 0;

    virtual Slopes tile_elevations() const = 0;

protected:
    static void add_triangles_based_on_model_details
        (Vector2I gridloc, Vector translation, const Slopes & slopes,
         EntityAndTrianglesAdder & adder);

    static const char * find_property(const char * name_, const TiXmlElement * properties);

    static Vector grid_position_to_v3(Vector2I r)
        { return Vector{r.x, 0, -r.y}; }

    SharedCPtr<Texture> common_texture() const
        { return m_texture_ptr; }

    std::array<Vector2, 4> common_texture_positions_from(Vector2I ts_r) const;

    Entity make_entity(Platform::ForLoaders & platform, Vector translation,
                       const SharedCPtr<RenderModel> & model_ptr) const;

    SharedCPtr<RenderModel> make_render_model_with_common_texture_positions
        (Platform::ForLoaders & platform,
         const Slopes & slopes,
         Vector2I loc_in_ts) const;

    Size2 common_texture_tile_size() const;

    Vector2 common_texture_origin(Vector2I ts_r) const;

private:
    SharedCPtr<Texture> m_texture_ptr;
    Size2 m_texture_size;
    Size2 m_tile_size;
};
