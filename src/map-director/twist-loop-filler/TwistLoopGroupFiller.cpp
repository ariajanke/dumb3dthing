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

#include "TwistLoopGroupFiller.hpp"

#include "../TileFactory.hpp"
#include "../TileSetPropertiesGrid.hpp"
#include "../twist-loop-filler.hpp"

#include "../../Components.hpp"
#include "../../TriangleSegment.hpp"
#include "../../platform.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

constexpr const auto k_twisty_origin = TwistyStripRadii::k_twisty_origin;

namespace {

using namespace cul::exceptions_abbr;
using cul::size_of, cul::top_left_of;

} // end of <anonymous> namespace

/* private */ void NorthSouthTwistTileGroup::load_
    (const RectangleI & rectangle, const TileSetXmlGrid &,
     Platform & platform)
{
    auto geo_grid = make_twisty_geometry_for
        (size_of(rectangle), TwistDirection::left, TwistPathDirection::north_south,
         CapTexturingAdapter{Vector2{top_left_of(rectangle)}, Size2{size_of(rectangle)}},
         k_breaks_per_segment);
    std::vector<Vertex> verticies;
    std::vector<unsigned> elements;
    ViewGridInserter<TriangleSegment> triangle_inserter{geo_grid.size2()};
    Grid<SharedPtr<const RenderModel>> render_models;
    render_models.set_size(geo_grid.size2(), nullptr);
    for (Vector2I r; r != geo_grid.end_position(); r = geo_grid.next(r)) {
        auto mod = platform.make_render_model();
        for (auto & triangle : geo_grid(r)) {
            triangle_inserter.push
                (TriangleSegment{triangle[0].position,
                                 triangle[1].position,
                                 triangle[2].position});
            for (auto & vtx : triangle) {
                verticies.push_back(vtx);
                elements.push_back(elements.size());
            }
        }
        triangle_inserter.advance();
        mod->load(verticies, elements);
        verticies.clear();
        elements.clear();
        render_models(r) = mod;
    }
    m_collision_triangles = ViewGrid<TriangleSegment>{std::move(triangle_inserter)};
    m_group_models = std::move(render_models);
}

void NorthSouthTwistTileGroup::operator ()
    (const Vector2I & position_in_group, const Vector2I & tile_offset,
     EntityAndTrianglesAdder & adder, Platform & platform) const
{
    // I forgot the formula
    auto v3_offset = TileFactory::grid_position_to_v3(tile_offset)
        + k_twisty_origin;
    for (auto & triangle : m_collision_triangles(position_in_group)) {
        adder.add_triangle(triangle.move(v3_offset));
    }
    auto entity = platform.make_renderable_entity();
    entity.add<SharedPtr<const RenderModel>, Translation, Visible>()
        = make_tuple
        (m_group_models(position_in_group), Translation{v3_offset}, Visible{});
}

RectangleI RectanglarGroupOfPred::get_rectangular_group_of
    (const Vector2I & start, const RectanglarGroupOfPred & is_in_group)
{
    auto x_end = start.x;
    for (; is_in_group(Vector2I{x_end, start.y}); ++x_end) {}
    if (x_end == start.x) {
        // this ain't right
        return RectangleI{};
    }

    for (int height = 0; true; ++height) {
        bool row_good = true;
        for (int x = start.x; x != x_end && row_good; ++x) {
            row_good = row_good && is_in_group(Vector2I{x, start.y + height});
        }

        if (!row_good) {
            return RectangleI{start, Size2I{x_end - start.x, height}};
        }
    }
}

void TwistLoopGroupFiller::load
    (const TileSetXmlGrid & xml_grid, Platform & platform)
{
    Grid<bool> checked;
    Grid<SharedPtr<TwistTileGroup>> tile_groups;
    checked    .set_size(xml_grid.size2(), false  );
    tile_groups.set_size(xml_grid.size2(), nullptr);
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        if (checked(r)) continue;
        if (xml_grid(r).type() != twist_loop_filler_names::k_ns_twist_loop) {
            checked(r) = true;
            continue;
        }
        auto origin_type = xml_grid(r).type();
        auto rect_group = get_rectangular_group_of(r,
            [&xml_grid, &checked] (const Vector2I & r)
        {
            if (!xml_grid.has_position(r) || checked(r)) return false;
            return xml_grid(r).type() == twist_loop_filler_names::k_ns_twist_loop;
        });
        // make a group
        auto twist_filler = make_shared<NorthSouthTwistTileGroup>();
        twist_filler->load(rect_group, xml_grid, platform);
        // std::fill doesn't work with containers
        auto checked_subgrid = cul::make_sub_grid(checked, rect_group);
        for (Vector2I r; r != checked_subgrid.end_position(); r = checked_subgrid.next(r)) {
            checked_subgrid(r) = true;
        }
        for (auto & ptr : cul::make_sub_grid(tile_groups, rect_group)) {
            ptr = twist_filler;
        }
    }
    m_tile_groups.swap(tile_groups);
}

UnfinishedTileGroupGrid TwistLoopGroupFiller::operator ()
    (const std::vector<TileLocation> & positions,
     UnfinishedTileGroupGrid && unfinished_group_grid) const
{
    UnfinishedProducableGroup<TwistLoopTile> producable_group;
    for (auto & pos : positions) {
        auto tile_group = m_tile_groups(pos.on_tileset);
        if (!tile_group) continue; // <- a baffling situation indeed!
        producable_group.at_location(pos.on_map).make_producable
            (pos.on_map, pos.on_tileset - tile_group->group_start(), tile_group);
    }
    unfinished_group_grid.add_group(std::move(producable_group));
    return std::move(unfinished_group_grid);
}
