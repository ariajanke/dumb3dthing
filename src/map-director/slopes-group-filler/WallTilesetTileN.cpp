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
            { return std::fmod(magnitude(t - 0.5), 1) + 0.5; }

        static Real interpolate_for_last(Real t) {
            t = interpolate_to_texture(t);
            return std::equal_to<Real>{}(t, 0) ? Real(1) : t;
        }

        static Vertex on_first_and_mid(Vertex vtx) {
            vtx.texture_position.x = interpolate_to_texture( kt_get_tx_x(vtx.position) );
            vtx.texture_position.y = interpolate_to_texture( kt_get_tx_y(vtx.position) );
            return vtx;
        }

        static Vertex on_last(Vertex vtx) {
            vtx.texture_position.x = interpolate_for_last( kt_get_tx_x(vtx.position) );
            vtx.texture_position.y = interpolate_for_last( kt_get_tx_y(vtx.position) );
            return vtx;
        }
    };

private:
    TriangleToVertexStrategies() {}
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

private:
    using VertexArray = std::array<Vertex, kt_capacity_in_triangles*3>;
    using VertexArrayIterator = typename VertexArray::iterator;

    TextureMappingStrategyFunction m_mapper_f = nullptr;
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

template <std::size_t kt_capacity_in_triangles>
class CollidablesCollection final {
public:
    void populate(const View<const Vertex *> & vertices) {
        auto count = vertices.end() - vertices.begin();
        if (count % 3 != 0) {
            throw RuntimeError{"number of vertices must be divisble by three"};
        } else if (count*3 > kt_capacity_in_triangles) {
            throw RuntimeError{"capacity exceeded"};
        }
        for (auto itr = vertices.begin(); itr != vertices.end(); itr += 3) {
            auto & a = *itr;
            auto & b = *(itr + 1);
            auto & c = *(itr + 2);
            m_elements[m_count++] =
                TriangleSegment{a.position, b.position, c.position};
        }
    }

    View<const TriangleSegment *> collidables() const
        { return View{&m_elements[0], &m_elements[0] + m_count}; }

private:
    std::array<TriangleSegment, kt_capacity_in_triangles> m_elements;
    std::size_t m_count = 0;
};


/* <! auto breaks BFS ordering !> */ auto
    make_get_next_for_dir_split_v
    (Vector end, Vector step)
{
    return [end, step] (Vector east_itr) {
        auto cand_next = east_itr + step;
        if (are_very_close(cand_next, end)) return cand_next;

        if (are_very_close(normalize(end - east_itr ),
                           normalize(end - cand_next)))
        { return cand_next; }
        return end;
    };
}

/* <! auto breaks BFS ordering !> */ auto make_step_factory(int step_count) {
    return [step_count](const Vector & start, const Vector & last) {
        auto diff = last - start;
        auto step = magnitude(diff) / Real(step_count);
        if (are_very_close(diff, Vector{})) return Vector{};
        return step*normalize(diff);
    };
}

} // end of <anonymous> namespace

void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using Triangle = TriangleSegment;
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }

    const auto make_step = make_step_factory(steps_count);

    auto itr_a = a_start;
    const auto next_a = make_get_next_for_dir_split_v(
        a_last, make_step(a_start, a_last));

    auto itr_b = b_start;
    const auto next_b = make_get_next_for_dir_split_v(
        b_last, make_step(b_start, b_last));

    while (   !are_very_close(itr_a, a_last)
           && !are_very_close(itr_b, b_last))
    {
        const auto new_a = next_a(itr_a);
        const auto new_b = next_b(itr_b);
        if (!are_very_close(itr_a, itr_b))
            add_triangle(Triangle{itr_a, itr_b, new_a});
        if (!are_very_close(new_a, new_b))
            add_triangle(Triangle{itr_b, new_a, new_b});
        itr_a = new_a;
        itr_b = new_b;
    }

    // at this point we are going to generate at most one triangle
    if (are_very_close(b_last, a_last)) {
        // here we're down to three points
        // there is only one possible triangle
        if (   are_very_close(itr_a, a_last)
            || are_very_close(itr_a, itr_b))
        {
            // take either being true:
            // in the best case: a line, so nothing
            return;
        }

        add_triangle(Triangle{itr_a, itr_b, a_last});
        return;
    }
    // a reminder from above
    assert(   are_very_close(itr_a, a_last)
           || are_very_close(itr_b, b_last));

    // here we still haven't ruled any points out
    if (   are_very_close(itr_a, itr_b)
        || (   are_very_close(itr_a, a_last)
            && are_very_close(itr_b, b_last)))
    {
        // either are okay, as they are "the same" pt
        return;
    } else if (!are_very_close(itr_a, a_last)) {
        // must exclude itr_b
        add_triangle(Triangle{itr_a, b_last, a_last});
        return;
    } else if (!are_very_close(itr_b, b_last)) {
        // must exclude itr_a
        add_triangle(Triangle{itr_b, a_last, b_last});
        return;
    }
}

// ----------------------------------------------------------------------------

NorthSouthSplit::NorthSouthSplit
    (const TileCornerElevations &,
     Real division_z) {}

NorthSouthSplit::NorthSouthSplit
    (Real north_west_y,
     Real north_east_y,
     Real south_west_y,
     Real south_east_y,
     Real division_z):
    m_div_nw(-0.5, north_west_y, -division_z),
    m_div_sw(-0.5, south_west_y, -division_z),
    m_div_ne( 0.5, north_east_y, -division_z),
    m_div_se( 0.5, south_east_y, -division_z)
{
    using cul::is_real;
    if (!is_real(south_west_y) || !is_real(south_east_y)) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    } else if (division_z < -0.5 || division_z > 0.5) {
        throw InvalidArgument
            {"north_south_split: division must be in [-0.5 0.5]"};
    }
}

void NorthSouthSplit::make_top(LinearStripTriangleCollection & collection) const {
    Vector sw{-0.5, south_west_y(), -0.5};
    Vector se{ 0.5, south_east_y(), -0.5};
    collection.make_strip(m_div_sw, sw, m_div_se, se, 1);
}

void NorthSouthSplit::make_bottom(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    Vector nw{-0.5, north_west_y(), 0.5};
    Vector ne{ 0.5, north_east_y(), 0.5};
    collection.make_strip(nw, m_div_nw, ne, m_div_ne, 1);
}

void NorthSouthSplit::make_wall(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    // both sets of y values' directions must be the same
    assert((north_east_y() - north_west_y())*
           (south_east_y() - south_west_y()) >= 0);
    collection.make_strip(m_div_nw, m_div_sw, m_div_ne, m_div_se, 1);
}

/* private */ void NorthSouthSplit::check_non_top_assumptions() const {
    using cul::is_real;
    if ((!is_real(north_west_y()) || !is_real(north_east_y()))) {
        throw InvalidArgument
            {"north_south_split: Northern elevations must be real numbers in "
             "top cases"};
    }
    if (south_west_y() < north_west_y() || south_east_y() < north_east_y()) {
        throw InvalidArgument
            {"north_south_split: method was designed assuming south is the top"};
    }
}

// ----------------------------------------------------------------------------

void WallTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const TilesetTileTexture & tile_texture,
     PlatformAssetsStrategy & platform)
{
    auto elevations = FlatTilesetTile::read_elevation_of(map_tileset_tile);
#   if 0 // keep for later (?)
    auto direction  = RampTileseTile ::read_direction_of(map_tileset_tile);
#   endif
    LimitedLinearStripCollection<2> col;
    col.set_texture_mapping_strategy(TriangleToVertexStrategies::lie_on_z_plane);
    NorthSouthSplit
        {k_inf, k_inf, *elevations->south_east(), *elevations->south_west(), 0.25}.
        make_top(col);
    ElementsCollection<2> elements_col;
    elements_col.populate(col.model_vertices());
    auto model = platform.make_render_model();
    model->load(col.model_vertices().begin(), col.model_vertices().end(),
                elements_col.elements().begin(), elements_col.elements().end());
    m_top_model = model;
    m_texture_ptr = tile_texture.texture();
    m_elevations = TileCornerElevations{{}, {}, elevations->south_east(), elevations->south_west()};
}

TileCornerElevations WallTilesetTile::corner_elevations() const {
    return m_elevations;
}

void WallTilesetTile::make
    (const TileCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{
    callbacks.
        add_entity_from_tuple(TupleBuilder{}.
            add(SharedPtr<const Texture>{m_texture_ptr}).
            add(SharedPtr<const RenderModel>{m_top_model}).
            finish());
    // the rest of everything is computed here, from geometry to models
}
