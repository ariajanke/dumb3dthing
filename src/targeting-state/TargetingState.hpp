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

#include "../Definitions.hpp"
#include "../targeting-state.hpp"

class TargetSeekerCone final {
public:
    TargetSeekerCone
        (const Vector & tip,
         const Vector & base,
         Real angle_range);

    bool contains(const Vector & pt) const;

    const Vector & base() const { return m_base; }

    const Vector & tip() const { return m_tip; }

    Real radius() const;

private:
    Vector m_tip;
    Vector m_base;
    Real m_angle_range;
    Real m_distance_range;
};

// ----------------------------------------------------------------------------

class TargetingState final : public TargetingState_ {
public:
    struct HighLow final {
        Real high;
        Real low;
    };

    // NOTE tightly coupled with TargetingState::position_of
    static HighLow interval_of(const TargetSeekerCone &);

    static Real position_of(const Vector &);

    TargetingState() {}

    void place_targetable(EntityRef, const Vector &);

    std::vector<EntityRef> find_targetables
        (const TargetSeekerCone &, std::vector<EntityRef> &&) const final;

    void update_on_scene(Scene &) final;

private:
    struct Target final {
        EntityRef entity_ref;
        Real position_on_line;
        Vector location;
    };

    static bool tuple_less_than(const Target &, const Target &);

    std::vector<Target> m_targets;
};
