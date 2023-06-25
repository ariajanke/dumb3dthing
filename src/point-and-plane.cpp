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

#include "point-and-plane.hpp"
#include "point-and-plane/DriverComplete.hpp"

#include <iostream>

namespace point_and_plane {

OnSegment::OnSegment
    (const SharedPtr<const TriangleFragment> & frag_,
     bool invert_norm_, Vector2 loc_, Vector2 dis_):
    fragment(frag_), segment(&frag_->segment()),
    invert_normal(invert_norm_), location(loc_), displacement(dis_)
{
    if (!segment->contains_point(loc_)) {
        std::cerr << loc_ << " "
                  << segment->point_a_in_2d() << " "
                  << segment->point_b_in_2d() << " "
                  << segment->point_c_in_2d() << std::endl;
    }
#   if 0
    assert(segment->contains_point(loc_));
#   endif
}

/* static */ UniquePtr<Driver> Driver::make_driver()
    { return UniquePtr<Driver>{make_unique<DriverComplete>()}; }

Vector location_of(const State & state) {
    auto * in_air = get_if<InAir>(&state);
    if (in_air) return in_air->location;
    auto & on_surf = std::get<OnSegment>(state);
    return on_surf.segment->point_at(on_surf.location);
}

Vector displaced_location_of(const State & state) {
    if (auto * on_air = get_if<InAir>(&state)) {
        return on_air->location + on_air->displacement;
    }
    auto & on_segment = std::get<OnSegment>(state);
    return on_segment.segment->point_at(  on_segment.location
                                        + on_segment.displacement);
}

Vector segment_displacement_to_v3(const State & state) {
    using std::get;
    auto pt_at = [&state](Vector2 r)
        { return get<OnSegment>(state).segment->point_at(r); };
    auto dis = get<OnSegment>(state).displacement;
    auto loc = get<OnSegment>(state).location;
    return pt_at(loc + dis) - pt_at(loc);
}

} // end of point_and_plane namespace
