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

#define mark_it mark_source_position(__LINE__, __FILE__).it

#include <ariajanke/cul/TreeTestSuite.hpp>

using namespace cul::exceptions_abbr;
#if 0
template <typename ExpType, typename Func>
cul::tree_ts::TestAssertion expect_exception(Func && f) {
    try {
        f();
    } catch (ExpType &) {
        return cul::tree_ts::test_that(true);
    }
    return cul::tree_ts::test_that(false);
}
#endif
