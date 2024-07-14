/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "targeting-state.hpp"
#include "targeting-state/TargetingState.hpp"

namespace {

Vector verify_direction(const Vector & r);
Real verify_distance_range(Real distance_range);
Real verify_angle_range(Real angle_range);

} // end of <anonymous> namespace

TargetSeeker::TargetSeeker():
    m_distance_range(0.01),
    m_angle_range(0.01*k_pi)
{}

TargetSeeker::TargetSeeker(Real distance_range, Real angle_range):
    m_distance_range(verify_distance_range(distance_range)),
    m_angle_range(verify_angle_range(angle_range))
{}

void TargetSeeker::set_facing_direction(const Vector & r)
    { m_direction = verify_direction(r); }

Real TargetSeeker::distance_range() const { return m_distance_range; }

Real TargetSeeker::angle_range() const { return m_angle_range; }

const Vector & TargetSeeker::direction() const
    { return m_direction; }

std::vector<EntityRef> TargetSeeker::find_targetables
    (const TargetsRetrieval & retrieval,
     const PpState & pp_state,
     std::vector<EntityRef> && collection) const
{
    auto location = point_and_plane::location_of(pp_state);
    TargetSeekerCone cone
        {location,
         location + m_direction*m_distance_range,
         m_angle_range};
    return retrieval.find_targetables(cone, std::move(collection));
}

// ----------------------------------------------------------------------------

/* static */ SharedPtr<TargetingState_> TargetingState_::make()
    { return std::make_shared<TargetingState>(); }

namespace {

Vector verify_direction(const Vector & r) {
    if (are_very_close(magnitude(r), 1))
        { return r; }
    throw InvalidArgument("direction must be a normal vector");
}

Real verify_distance_range(Real distance_range) {
    if (distance_range > 0 && cul::is_real(distance_range))
        { return distance_range; }
    throw InvalidArgument("distance range must be in (0 infinity)");
}

Real verify_angle_range(Real angle_range) {
    if (angle_range > 0 && angle_range < k_pi/2.)
        { return angle_range; }
    throw InvalidArgument("angle range must be in (0 pi/2)");
}

} // end of <anonymous> namespace
