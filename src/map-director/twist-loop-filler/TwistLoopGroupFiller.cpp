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
#include "../TilesetPropertiesGrid.hpp"
#include "../twist-loop-filler.hpp"

#include "../../Components.hpp"
#include "../../TriangleSegment.hpp"
#include "../../platform.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

constexpr const auto k_twisty_origin = TwistyStripRadii::k_twisty_origin;

namespace {

using cul::size_of, cul::top_left_of;

} // end of <anonymous> namespace

/* private */ void NorthSouthTwistTileGroup::load_(const RectangleI & rectangle) {
    const auto & rect_size = size_of(rectangle);
    CapTexturingAdapter txadapter{Vector2{top_left_of(rectangle)}, Size2{size_of(rectangle)}};
    auto geo_grid = make_twisty_geometry_for
        (rect_size, TwistDirection::left, TwistPathDirection::north_south,
         txadapter, k_breaks_per_segment);
    std::vector<Vertex> vertices;
    std::vector<unsigned> elements;
    ViewGridInserter<TriangleSegment> triangle_inserter{geo_grid.size2()};
    Grid<ElementsVerticesPair> elements_and_vertices;
    elements_and_vertices.set_size(geo_grid.size2(), ElementsVerticesPair{});
    for (Vector2I r; r != geo_grid.end_position(); r = geo_grid.next(r)) {
        for (auto & triangle : geo_grid(r)) {
            TriangleSegment triangle_segment
                {triangle[0].position, triangle[1].position,
                 triangle[2].position};
            triangle_inserter.push(triangle_segment);
            for (auto & vtx : triangle) {
                vertices.push_back(vtx);
                elements.push_back(elements.size());
            }
        }
        triangle_inserter.advance();
        elements_and_vertices(r).vertices.swap(vertices);
        elements_and_vertices(r).elements.swap(elements);
        vertices.clear();
        elements.clear();
    }
    m_collision_triangles = triangle_inserter.finish();
    m_elements_vertices   = std::move(elements_and_vertices);
}

void NorthSouthTwistTileGroup::operator ()
    (const Vector2I & position_in_group, const Vector2I &,
     ProducableTileCallbacks & callbacks) const
{
#   if 0
    // I forgot the formula
    auto v3_offset = TileFactory::grid_position_to_v3(tile_offset)
        + k_twisty_origin;
#   endif
    for (auto & triangle : m_collision_triangles(position_in_group)) {
        callbacks.add_collidable(triangle.move(k_twisty_origin));
    }
#   if 0
    for (auto & triangle : m_collision_triangles(position_in_group)) {
        callbacks.add(triangle.move(v3_offset));
    }
    // LoD x2
    auto mod = callbacks.platform().make_render_model();
    auto & [elements, vertices] = m_elements_vertices(position_in_group);
    mod->load(elements, vertices);
    auto entity = callbacks.platform().make_renderable_entity();
    entity.add<SharedPtr<const RenderModel>, Translation, Visible>()
        = make_tuple(mod, Translation{v3_offset}, Visible{});
#   else
    auto mod = callbacks.make_render_model();
    auto & [elements, vertices] = m_elements_vertices(position_in_group);
    mod->load(elements, vertices);
    auto e = callbacks.
        add_entity<SharedPtr<const RenderModel>, ModelVisibility>
        (std::move(mod), ModelVisibility{});
    e.get<ModelTranslation>() += k_twisty_origin;
#   endif
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

/* static */ Size2I TwistLoopGroupFiller::size_of_map
    (const std::vector<TileLocation> & tile_locations)
{
    Vector2I max_location;
    for (const auto & loc : tile_locations) {
        using std::max;
        max_location.x = max(loc.on_map.x, max_location.x);
        max_location.y = max(loc.on_map.y, max_location.y);
    }
    return Size2I{max_location.x + 1, max_location.y + 1};
}

/* static */ Grid<bool> TwistLoopGroupFiller::map_positions_to_grid
    (const std::vector<TileLocation> & tile_locations)
{
    Grid<bool> active_tiles;
    active_tiles.set_size(size_of_map(tile_locations), false);
    for (const auto & loc : tile_locations) {
        active_tiles(loc.on_map) = true;
    }
    return active_tiles;
}

void TwistLoopGroupFiller::load
    (const TilesetXmlGrid &, Platform &)
{
#   if 0
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
        // wrong, no the right step here...

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
#   endif
}
#if 0
ProducableGroupTileLayer TwistLoopGroupFiller::operator ()
    (const std::vector<TileLocation> & positions,
     ProducableGroupTileLayer && unfinished_group_grid) const
{
    Grid<SharedPtr<TwistTileGroup>> group_grid;
    group_grid = [&positions] {
        Grid<SharedPtr<TwistTileGroup>> group_grid;
        group_grid.set_size(size_of_map(positions), nullptr);
        auto unchecked = map_positions_to_grid(positions);
        for (Vector2I r; r != unchecked.end_position(); r = unchecked.next(r)) {
            if (!unchecked(r))
                { continue; }
            auto rect = get_rectangular_group_of(r, [&unchecked] (const Vector2I & v) {
                if (unchecked.has_position(v)) return bool(unchecked(v));
                return false;
            });

            auto twist_filler = make_shared<NorthSouthTwistTileGroup>();
            twist_filler->load(rect);
            {
            auto unchecked_subg = cul::make_sub_grid(unchecked, rect);
            for (Vector2I v; v != unchecked_subg.end_position(); v = unchecked_subg.next(v)) {
                unchecked_subg(v) = false;
            }
            }
#           if 0
            for (auto & bref : cul::make_sub_grid(unchecked, rect))
                { bref = false; }
#           endif
            for (auto & filler : cul::make_sub_grid(group_grid, rect))
                { filler = twist_filler; }
        }
        return group_grid;
    } ();
    UnfinishedProducableGroup<TwistLoopTile> producable_group;
    for (auto & pos : positions) {
        const auto & tile_group = group_grid(pos.on_map);
        if (!tile_group) continue;
        producable_group.at_location(pos.on_map).make_producable
            (pos.on_map, pos.on_map - tile_group->group_start(), tile_group);
    }
    unfinished_group_grid.add_group(std::move(producable_group));
    return std::move(unfinished_group_grid);
#   if 0
    UnfinishedProducableGroup<TwistLoopTile> producable_group;
    for (auto & pos : positions) {
        auto tile_group = m_tile_groups(pos.on_tileset);
        if (!tile_group) continue; // <- a baffling situation indeed!
        producable_group.at_location(pos.on_map).make_producable
            (pos.on_map, pos.on_tileset - tile_group->group_start(), tile_group);
    }
    unfinished_group_grid.add_group(std::move(producable_group));
    return std::move(unfinished_group_grid);
#   endif
}
#endif
