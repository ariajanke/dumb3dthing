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

#include "TargetingState.hpp"

namespace {

using HighLow = TargetingState::HighLow;

} // end of <anonymous> namespace

TargetSeekerCone::TargetSeekerCone
    (const Vector & tip,
     const Vector & base,
     Real length,
     Real angle_range):
    m_tip(tip),
    m_base(base),
    m_length(length),
    m_angle_range(angle_range)
{}

bool TargetSeekerCone::contains(const Vector & pt) const {
    auto r = m_base - m_tip;
    auto v = pt     - m_tip;
    return cul::angle_between(r, v) < m_angle_range;
}

Real TargetSeekerCone::radius() const {
    return std::sin(m_angle_range)*m_length;
}

// ----------------------------------------------------------------------------

/* static */ HighLow TargetingState::interval_of(const TargetSeekerCone & cone) {
    const auto & base = cone.base();
    const auto & tip = cone.tip();
    auto radius = cone.radius();
    auto dir_on_plane = cul::project_onto_plane(base - tip, k_north);
    auto dir_on_line  = cul::project_onto(dir_on_plane, k_east);
    auto ex_a = base + dir_on_line*radius;
    auto ex_b = base - dir_on_line*radius;
    // NOTE specific to projecting on east (x coordinate)
    Real low = k_inf;
    Real high = -k_inf;
    for (auto r : { tip, ex_a, ex_b }) {
        low  = std::min(low , r.x);
        high = std::max(high, r.x);
    }
    HighLow rv;
    rv.high = high;
    rv.low = low;
    if (!cul::is_real(rv.high) || !cul::is_real(rv.low)) {
        throw RuntimeError("Interval not finite");
    }
    return rv;
}

/* static */ Real TargetingState::position_of(const Vector & r)
//  on "east-west" axis
    { return r.x; }

/* static private */ bool TargetingState::tuple_less_than
    (const Target & lhs, const Target & rhs)
{ return lhs.position_on_line < rhs.position_on_line; }

void verify_real(const Vector & r) {
    if (!cul::is_real(r)) {
        throw InvalidArgument{"location must be a real vector"};
    }
}

void TargetingState::place_targetable(EntityRef ref, const Vector & location) {
    verify_real(location);
    Target target;
    target.entity_ref = ref;
    target.location = location;
    target.position_on_line = position_of(location);
    m_targets.push_back(target);
}

std::vector<EntityRef> TargetingState::find_targetables
    (const TargetSeekerCone & view_cone,
     std::vector<EntityRef> && target_collection) const
{
    target_collection.clear();
    auto interval = interval_of(view_cone);
    auto beg = std::lower_bound
        (m_targets.begin(),
         m_targets.end(),
         interval,
         [] (const Target & lhs, const HighLow & rhs) {
            return lhs.position_on_line < rhs.low;
         });
    for (auto itr = beg; itr != m_targets.end(); ++itr) {
        if (itr->position_on_line > interval.high) {
            break;
        } else if (view_cone.contains(itr->location)) {
            target_collection.push_back(itr->entity_ref);
        }
    }

    return std::move(target_collection);
}

void TargetingState::update_on_scene(Scene & scene) {
    m_targets.clear();
    for (auto & ent : scene) {
        if (!ent.has_all<TargetComponent, PpState>())
            { continue; }
        const auto & pp_state = ent.get<PpState>();
        place_targetable
            (ent.as_reference(), point_and_plane::location_of(pp_state));
    }
    std::sort(m_targets.begin(), m_targets.end(), tuple_less_than);
}
