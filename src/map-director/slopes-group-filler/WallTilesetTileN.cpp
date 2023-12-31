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

#include "WallTilesetTileN.hpp"
#include "FlatTilesetTileN.hpp"
#include "RampTilesetTileN.hpp"

#include "../../TriangleSegment.hpp"

#include <numeric>

namespace {

class TriangleToVertexStrategies {
public:
    using VertexTriangle = std::array<Vertex, 3>;
    using StrategyFunction = VertexTriangle(*)(const TriangleSegment &);

    static VertexTriangle all_zeroed(const TriangleSegment & triangle) {
        return spacial_position_populated_vertices(triangle);
    }

    static VertexTriangle lie_on_y_plane(const TriangleSegment & triangle) {
        return texture_positioned_vertices_from_spacial<get_x, get_z>
            (spacial_position_populated_vertices(triangle));
    }

    static VertexTriangle lie_on_x_plane(const TriangleSegment & triangle) {
        return texture_positioned_vertices_from_spacial<get_y, get_z>
            (spacial_position_populated_vertices(triangle));
    }

    static VertexTriangle lie_on_z_plane(const TriangleSegment & triangle) {
        return texture_positioned_vertices_from_spacial<get_x, get_y>
            (spacial_position_populated_vertices(triangle));
    }

    static VertexTriangle spacial_position_populated_vertices
        (const TriangleSegment & triangle)
    {
        VertexTriangle rv;
        rv[0].position = triangle.point_a();
        rv[1].position = triangle.point_b();
        rv[2].position = triangle.point_c();
        return rv;
    }

    template <Real(*kt_get_tx_x)(const Vector &), Real(*kt_get_tx_y)(const Vector &)>
    static VertexTriangle texture_positioned_vertices_from_spacial
        (VertexTriangle triangle)
    {
        return TexturePositioner<kt_get_tx_x, kt_get_tx_y>::
            texture_positioned_vertices_from_spacial(triangle);
    }

private:
    static Real get_x(const Vector & r) { return r.x; }

    static Real get_y(const Vector & r) { return r.y; }

    static Real get_z(const Vector & r) { return r.z; }

    template <Real(*kt_get_tx_x)(const Vector &), Real(*kt_get_tx_y)(const Vector &)>
    class TexturePositioner final {
    public:
        static VertexTriangle texture_positioned_vertices_from_spacial
            (VertexTriangle triangle)
        {
            triangle[0] = on_first_and_mid(triangle[0]);
            triangle[1] = on_first_and_mid(triangle[1]);
            triangle[2] = on_last(triangle[2]);
            return triangle;
        }

    private:
        static Real interpolate_to_texture(Real t)
            { return std::min(std::max(t + 0.5, 0.), 1.); }

        static Real interpolate_for_last(Real t) {
            return interpolate_to_texture(t);
        }

        static Vertex on_first_and_mid(Vertex vtx) {
            vtx.texture_position.x = interpolate_to_texture( kt_get_tx_x(vtx.position) );
            vtx.texture_position.y = 1 - interpolate_to_texture( kt_get_tx_y(vtx.position) );
            return vtx;
        }

        static Vertex on_last(Vertex vtx) {
            vtx.texture_position.x = interpolate_for_last( kt_get_tx_x(vtx.position) );
            vtx.texture_position.y = 1 - interpolate_for_last( kt_get_tx_y(vtx.position) );
            return vtx;
        }
    };

private:
    TriangleToVertexStrategies() {}
};

template <std::size_t kt_capacity_in_triangles>
class CollidablesCollection final : public LinearStripTriangleCollection {
public:
    void append(const View<const Vertex *> & vertices) {
        auto count = vertices.end() - vertices.begin();
        if (count % 3 != 0) {
            throw RuntimeError{"number of vertices must be divisble by three"};
        }
        for (auto itr = vertices.begin(); itr != vertices.end(); itr += 3) {
            auto & a = *itr;
            auto & b = *(itr + 1);
            auto & c = *(itr + 2);
            verify_capacity_for_another();
            m_elements[m_count++] =
                TriangleSegment{a.position, b.position, c.position};
        }
    }

    void add_triangle(const TriangleSegment & triangle) final {
        verify_capacity_for_another();
        m_elements[m_count++] = triangle;
    }

    View<const TriangleSegment *> collidables() const
        { return View{&m_elements[0], &m_elements[0] + m_count}; }

private:
    void verify_capacity_for_another() const {
        if (m_count + 1 > kt_capacity_in_triangles*3) {
            throw RuntimeError{"capacity exceeded"};
        }
    }
    std::array<TriangleSegment, kt_capacity_in_triangles> m_elements;
    std::size_t m_count = 0;
};

// I need vertices, and collidable triangles
template <std::size_t kt_capacity_in_triangles>
class LimitedLinearStripCollection final : public LinearStripTriangleCollection {
public:
    using VertexTriangle = TriangleToVertexStrategies::VertexTriangle;
    using TextureMappingStrategyFunction = TriangleToVertexStrategies::StrategyFunction;

    void set_texture_mapping_strategy(TextureMappingStrategyFunction f)
        { m_mapper_f = f; }

    void add_triangle(const TriangleSegment & triangle) final {
        if (m_array.end() - m_end < 3) {
            throw RuntimeError{"capacity exceeded"};
        }
        for (const Vertex & vtx : m_mapper_f(triangle)) {
            *m_end++ = vtx;
        }
    }

    View<const Vertex *> model_vertices() const
        { return View{&m_array[0], &m_array[0] + (m_end - m_array.begin())}; }

    // need something better than this...
    void fit_to_texture(const TilesetTileTexture & tileset_tile_texture) {
        for (auto itr = m_array.begin(); itr != m_end; ++itr) {
            *itr = tileset_tile_texture.interpolate(*itr);
        }
    }

private:
    using VertexArray = std::array<Vertex, kt_capacity_in_triangles*3>;
    using VertexArrayIterator = typename VertexArray::iterator;

    TextureMappingStrategyFunction m_mapper_f = TriangleToVertexStrategies::all_zeroed;
    VertexArray m_array;
    VertexArrayIterator m_end = m_array.begin();
};

template <std::size_t kt_capacity_in_triangles>
class ElementsCollection final {
public:
    void populate(const View<const Vertex *> & vertices) {
        auto count = vertices.end() - vertices.begin();
        if (count > kt_capacity_in_triangles*3) {
            throw RuntimeError{"capacity exceeded"};
        }
        std::iota(m_elements.begin(), m_elements.begin() + count, 0);
        m_count = count;
    }

    View<const unsigned *> elements() const
        { return View{&m_elements[0], &m_elements[0] + m_count}; }

private:
    std::array<unsigned, kt_capacity_in_triangles*3> m_elements;
    std::size_t m_count = 0;
};

template <std::size_t kt_triangle_count>
SharedPtr<const RenderModel> make_model
    (LimitedLinearStripCollection<kt_triangle_count> & linear_strip,
     SharedPtr<RenderModel> && new_model)
{
    ElementsCollection<kt_triangle_count> elements_col;
    elements_col.populate(linear_strip.model_vertices());
    new_model->load
        (linear_strip.model_vertices().begin(),
         linear_strip.model_vertices().end(),
         elements_col.elements().begin(),
         elements_col.elements().end());
    return new_model;
}

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

void WallTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const TilesetTileTexture & tile_texture,
     PlatformAssetsStrategy & platform)
{
    auto elevations = FlatTilesetTile::read_elevation_of(map_tileset_tile)->
        add(TileCornerElevations{1, 1, 1, 1});
    auto direction  = RampTileseTile ::read_direction_of(map_tileset_tile);
    m_startegy = &m_strategy_source(*direction);

    LimitedLinearStripCollection<2> col;
    choose_on_direction
        (elevations,
         -0.25,
         [&col] (const TwoWaySplit & two_way_split) {
             col.set_texture_mapping_strategy(TriangleToVertexStrategies::lie_on_y_plane);
             two_way_split.make_top(col);
         });
    col.fit_to_texture(tile_texture);
    // and you can inject a strategy
    // cost is like a couple of virtual calls
    // benefits: less code/overhead and probably easier to test too!
    auto model = make_model(col, platform.make_render_model());
    m_top_model = model;
    m_tileset_tile_texture = tile_texture;
    m_elevations = m_startegy->filter_to_known_corners(elevations);
}

TileCornerElevations WallTilesetTile::corner_elevations() const {
    return m_elevations;
}

void WallTilesetTile::make
    (const NeighborCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{
    callbacks.
        add_entity_from_tuple(TupleBuilder{}.
            add(SharedPtr<const Texture>{m_tileset_tile_texture.texture()}).
            add(SharedPtr<const RenderModel>{m_top_model}).
            finish());
    // the rest of everything is computed here, from geometry to models
    auto computed_elevations = m_elevations.value_or(neighboring_elevations);
    CollidablesCollection<6> col_col;
    LimitedLinearStripCollection<4> col;
    choose_on_direction
        (computed_elevations,
         -0.25,
         [&col, &col_col] (const TwoWaySplit & splitter) {
             // how do I set texture strategies here?
             // how would I go about generating stuff for corner walls?
             col.set_texture_mapping_strategy(TriangleToVertexStrategies::lie_on_z_plane);
             splitter.make_wall(col);
             splitter.make_wall(col_col);
             col.set_texture_mapping_strategy(TriangleToVertexStrategies::lie_on_y_plane);
             splitter.make_bottom(col);
             splitter.make_bottom(col_col);
             splitter.make_top(col_col);
         });
    for (auto & tri : col_col.collidables()) {
        callbacks.add_collidable(tri);
    }
    // this is not good enough, don't we want to texture the wall parts
    // differently?
    col.fit_to_texture(m_tileset_tile_texture);
    auto model = make_model(col, callbacks.make_render_model());
    callbacks.
        add_entity_from_tuple(TupleBuilder{}.
            add(SharedPtr<const RenderModel>{model}).
            add(SharedPtr<const Texture>{m_tileset_tile_texture.texture()}).
            finish());
}
