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

#include "../src/TasksController.hpp"

#include "../src/map-director/ViewGrid.hpp"

#include "../test-helpers.hpp"

struct Counted final {
public:
    Counted(){}
    Counted(const Counted &) {}
    Counted(Counted &&) {}
    ~Counted() {}

    static int count() { return s_count; }
    static void reset_count() { s_count = 0; }
private:
    static int s_count;
};

/* static */ int Counted::s_count = 0;

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<ViewGridInserter<int>>("ViewGridInserter #advance -> #filled")([] {
    ViewGridInserter<int> inserter{2, 2};
    mark_it("fills after 2x2 advances", [&] {
        for (int i = 0; i != 4; ++i) {
            inserter.advance();
        }
        return test_that(inserter.filled());
    });
    mark_it("throws a runtime exception if overfilled", [&] {
        return expect_exception<RtError>([&] {
            for (int i = 0; i != 10000; ++i) {
                inserter.advance();
            }
        });
    });
    mark_it("creates a grid which matches the inserter's size", [&] {
        for (int i = 0; i != 4; ++i) {
            inserter.advance();
        }
        auto [cont, grid] = inserter.move_out_container_and_grid_view();
        return test_that(grid.size2() == Size2I{2, 2});
    });
    mark_it("creates an empty container, as no elements where pushed", [&] {
        for (int i = 0; i != 4; ++i) {
            inserter.advance();
        }
        auto [cont, grid] = inserter.move_out_container_and_grid_view();
        return test_that(cont.empty());
    });
});
struct ViewGridLifetimes final {};
describe<ViewGridLifetimes>("ViewGridInserter #push -> #move_out...")([] {
    Counted::reset_count();
    mark_it("before any push, there are no counted objects", [&] {
        return test_that(Counted::count() == 0);
    });

    ViewGridInserter<Counted> inserter{2, 2};
    inserter.push(Counted{});
    inserter.advance();
    inserter.advance();
    inserter.push(Counted{});
    inserter.push(Counted{});
    inserter.advance();
    inserter.push(Counted{});
    inserter.advance();

    mark_it("before moving out there are four counted objects", [&] {
        return test_that(Counted::count() == 4);
    });
    auto [cont, grid] = inserter.move_out_container_and_grid_view();
    mark_it("after moving out there are four counted objects", [&] {
        return test_that(Counted::count() == 4);
    });
});

struct ViewGridPositions final {};
describe<ViewGridPositions>("ViewGridInserter #position").
    depends_on<ViewGridLifetimes>()([]
{
    Grid<int> sample_grid;
    sample_grid.set_size(2, 2, 0);
    ViewGridInserter<int> inserter{sample_grid.size2()};
    auto inserter_pos = inserter.position();
    Vector2I grid_pos;
    mark_it("returns position of first grid element, before any advance", [&] {
        return test_that(grid_pos == inserter_pos);
    });

    // v gets repeated... but eeeeeeeeehhhhhhhhhhhh
    inserter.advance();
    inserter_pos = inserter.position();
    grid_pos = sample_grid.next(grid_pos);
    mark_it("single advance follows cul grid's positions", [&] {
        return test_that(grid_pos == inserter_pos);
    });
    inserter.advance();
    inserter.advance();
    grid_pos = sample_grid.next(grid_pos);
    grid_pos = sample_grid.next(grid_pos);
    mark_it("multiple advances follow cul grid's positions", [&] {
        return test_that(grid_pos == inserter_pos);
    });
});
describe<ViewGridInserter<int>>
    ("ViewGridInserter #advance -> #filled (with elements)").
    depends_on<ViewGridPositions>()([]
{
    ViewGridInserter<int> inserter{2, 2};
    auto two_els_position = inserter.position();
    inserter.push(1);
    inserter.push(2);
    inserter.advance();
    auto no_els_position = inserter.position();
    inserter.advance();
    auto one_els_position = inserter.position();
    inserter.push(3);
    inserter.advance();
    inserter.push(4);
    inserter.advance();

    auto [cont_, grid_] = inserter.move_out_container_and_grid_view();
    auto & cont = cont_;
    auto & grid = grid_;
    mark_it("creates a container with four (pushed) elements", [&] {
        return test_that(cont.size() == 4);
    });
    mark_it("creates grid, with a view of two elements at +0", [&] {
        auto view = grid(two_els_position);
        return test_that(view.end() - view.begin() == 2);
    });
    mark_it("creates grid, with a view of zero elements at +1", [&] {
        auto view = grid(no_els_position);
        return test_that(view.end() == view.begin());
    });
    mark_it("creates grid, with a view on one element at +2", [&] {
        auto view = grid(one_els_position);
        return test_that(view.end() - view.begin() == 1);
    });
});

describe<ViewGridInserter<int>>
    ("ViewGridInserter #transform_values")([]
{
    ViewGridInserter<int> inserter{2, 2};
    inserter.push(1);
    inserter.advance();
    auto c_pos = inserter.position();
    inserter.push(2);
    inserter.advance();
    inserter.push(3);
    inserter.advance();
    mark_it("transforms values successfully for a partialy finished grid view", [&] {
        auto char_inserter = inserter.transform_values<char>([] (int i) { return 'A' + i; });
        char_inserter.advance();
        if (!char_inserter.filled()) return test_that(false);
        auto [cont, grid] = char_inserter.move_out_container_and_grid_view();
        return test_that(*grid(c_pos).begin() == 'C');
    });
    auto e_pos = inserter.position();
    inserter.push(4);
    inserter.advance();
    mark_it("transforms values successfully for a finished grid view", [&] {
        auto char_inserter = inserter.transform_values<char>([] (int i) { return 'A' + i; });
        if (!char_inserter.filled()) return test_that(false);
        auto [cont, grid] = char_inserter.move_out_container_and_grid_view();
        return test_that(*grid(e_pos).begin() == 'E');
    });
});

return 1;

} ();
