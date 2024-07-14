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

Real stray_portion_of(const TargetSeekerCone & cone);

} // end of <anonymous> namespace

TargetSeekerCone::TargetSeekerCone
    (const Vector & tip,
     const Vector & base,
     Real angle_range):
    m_tip(tip),
    m_base(base),
    m_angle_range(angle_range),
    m_distance_range(magnitude(m_base - m_tip))
{}

bool TargetSeekerCone::contains(const Vector & pt) const {
    auto r = m_base - m_tip;
    auto v = pt     - m_tip;
    auto angle = angle_between(r, v);
    return angle < m_angle_range &&
           magnitude(project_onto(v, r)) < m_distance_range;
}

Real TargetSeekerCone::radius() const {
    return std::tan(m_angle_range)*m_distance_range;
}

// ----------------------------------------------------------------------------

/* static */ HighLow TargetingState::interval_of(const TargetSeekerCone & cone) {
    Real stray_portion = stray_portion_of(cone);

    const auto & tip = cone.tip();
    auto stray = stray_portion*cone.radius();
    auto proj_base = project_onto(cone.base(), k_east);
    // v extreme points
    auto ex_a = proj_base + k_east*stray;
    auto ex_b = proj_base - k_east*stray;
    HighLow rv;
    rv.low  =  k_inf;
    rv.high = -k_inf;
    for (auto r : { tip, ex_a, ex_b }) {
        rv.low  = std::min(rv.low , r.x);
        rv.high = std::max(rv.high, r.x);
    }

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

namespace {

Real stray_portion_of(const TargetSeekerCone & cone) {
    const auto & base = cone.base();
    const auto & tip = cone.tip();
    auto norm = base - tip;
    auto num = dot(base, norm);
    auto denom = dot(norm, k_east);
    if (are_very_close(0., denom)) {
        // they're orthogonal, maximum effect
        return 1.;
    }
    auto intersection = k_east*(num / denom);
    if (are_very_close(base, intersection)) {
        return std::sin(angle_between(tip - base, k_east));
    }
    return std::cos(angle_between(base - intersection, k_east));
}

} // end of <anonymous> namespace
