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

#include "../../src/map-director/RegionEdgeConnectionsContainer.hpp"
#include "../../src/TriangleLink.hpp"

#include "../test-helpers.hpp"

#include <unordered_set>

namespace {

struct Samp final {
    SharedPtr<TriangleLink> w;
    SharedPtr<TriangleLink> e;
    ViewGrid<SharedPtr<TriangleLink>> view_grid;
};

Samp make_view_grid_for_tile(const Vector2I & r) {
    auto link_w = make_shared<TriangleLink>
        (Vector{Real(r.x), 0., Real(r.y)},
         Vector{Real(r.x + 1), 0., Real(r.y)},
         Vector{Real(r.x), 0., Real(r.y + 1)});
    auto link_e = make_shared<TriangleLink>
        (Vector{Real(r.x + 1), 0., Real(r.y)},
         Vector{Real(r.x + 1), 0., Real(r.y + 1)},
         Vector{Real(r.x), 0., Real(r.y + 1)});
    ViewGridInserter<SharedPtr<TriangleLink>> grid_inserter{Size2I{1, 1}};
    grid_inserter.push(link_w);
    grid_inserter.push(link_e);
    grid_inserter.advance();
    Samp rv;
    rv.w = link_w;
    rv.e = link_e;
    rv.view_grid = grid_inserter.finish();
    return rv;
}

ScaledTriangleViewGrid make_scaled_triangle_view_grid
    (const ViewGrid<SharedPtr<TriangleLink>> & triangle_grid)
{
    using ViewGridTriangle = ViewGrid<SharedPtr<TriangleLink>>;
    return ScaledTriangleViewGrid{make_shared<ViewGridTriangle>(triangle_grid), ScaleComputation{}};
}

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

struct Whatevs final {};

describe<Whatevs>("RegionEdgeConnectionsContainer")([] {
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    auto samp_0_0_old = make_view_grid_for_tile(Vector2I{});
    auto samp_1_0     = make_view_grid_for_tile(Vector2I{1, 0});
    auto samp_0_0_new = make_view_grid_for_tile(Vector2I{});

    auto samp_0_0_old_ptr = make_scaled_triangle_view_grid(samp_0_0_old.view_grid);

    RegionEdgeConnectionsAdder adder_first;
    RegionEdgeConnectionsContainer cont_first;
    RegionEdgeConnectionsRemover remover_first;

    auto usec00_old = samp_0_0_old.e.use_count();
    auto usec10     = samp_1_0    .e.use_count();
    auto usec00_new = samp_0_0_new.e.use_count();

    adder_first.add(Vector2I{    }, samp_0_0_old_ptr);
    {
        auto samp_1_0_ptr = make_scaled_triangle_view_grid(samp_1_0.view_grid);
        adder_first.add(Vector2I{1, 0}, samp_1_0_ptr    );
    }
    cont_first = adder_first.finish();

    mark_it("adding links to container, container owns the given links", [&] {        
        // triangles neighbor four axises with 1x1 grids
        return test_that(samp_0_0_old.e.use_count() - usec00_old == 4 &&
                         samp_1_0    .e.use_count() - usec10 == 4   );
    }).
    next([&] {
        remover_first = cont_first.make_remover();
        remover_first.remove_region(Vector2I{}, samp_0_0_old_ptr);
        cont_first = remover_first.finish();
    }).
    mark_it("then remove one region of links, the container no longer owns the links", [&] {
        auto a = samp_0_0_old.e.use_count();
        auto b = samp_1_0    .e.use_count();
        return test_that(samp_0_0_old.e.use_count() - usec00_old == 0 &&
                         samp_1_0    .e.use_count() - usec10     == 4   );
    }).
    next([&] {
        auto samp_0_0_new_ptr = make_scaled_triangle_view_grid(samp_0_0_new.view_grid);
        adder_first = cont_first.make_adder();
        adder_first.add(Vector2I{}, samp_0_0_new_ptr);
        cont_first = adder_first.finish();
    }).
    mark_it("then add new links into the same region", [&] {
        return test_that(samp_0_0_old.e.use_count() - usec00_old == 0 &&
                         samp_1_0    .e.use_count() - usec10     == 4 &&
                         samp_0_0_new.e.use_count() - usec00_new == 4);
    });

});

return [] {};

} ();
