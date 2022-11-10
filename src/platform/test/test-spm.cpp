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

#include "test-functions.hpp"

#include "../../SpatialPartitionMap.hpp"

#include <ariajanke/cul/TestSuite.hpp>

bool run_spm_tests() {
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("ProjectionLine");
    suite.start_series("SpatialDivisionPairs");
    suite.start_series("SpatialPartitionMapHelpers");
    suite.start_series("SpatialPartitionMap");
    suite.start_series("ProjectedSpatialMap");

    return suite.has_successes_only();
}
