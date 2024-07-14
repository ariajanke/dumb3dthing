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

#pragma once

#include "Definitions.hpp"
#include "point-and-plane.hpp"

class TargetSeekerCone;

class TargetComponent final {};

class TargetsRetrieval {
public:
    virtual ~TargetsRetrieval() {}

    virtual std::vector<EntityRef> find_targetables
        (const TargetSeekerCone &, std::vector<EntityRef> &&) const = 0;
};

class TargetSeeker final {
public:
    TargetSeeker();

    /// @distance_range in [0, infinity)
    /// @angle_range in (0, pi/2)
    TargetSeeker(Real distance_range, Real angle_range);

    void set_facing_direction(const Vector &);

    Real distance_range() const;

    Real angle_range() const;

    const Vector & direction() const;

    std::vector<EntityRef> find_targetables
        (const TargetsRetrieval &,
         const PpState &,
         std::vector<EntityRef> &&) const;

private:
    Vector m_direction = k_up;
    Real m_distance_range;
    Real m_angle_range;
};


class TargetingState_ : public TargetsRetrieval {
public:
    static SharedPtr<TargetingState_> make();

    virtual void update_on_scene(Scene &) = 0;
};
