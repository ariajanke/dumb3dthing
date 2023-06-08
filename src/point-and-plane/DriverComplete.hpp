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

#include "../point-and-plane.hpp"
#include "FrameTimeLinkContainer.hpp"

namespace point_and_plane {

// this can become a bottle neck in performance
// (as can entity component accessors)
// so triangles are then sorted along an arbitrary axis
// I should like to chose a line wherein triangles are most widely and evenly
// distrubuted to reduce load.
//
// class does too much
// maybe top level it's like a controller
class DriverComplete final : public Driver {
public:
    void add_triangle(const SharedPtr<const TriangleLink> &) final;

    void remove_triangle(const SharedPtr<const TriangleLink> &) final;

    void clear_all_triangles() final;

    Driver & update() final;

    State operator () (const State &, const EventHandler &) const final;

private:
    // the job of each method here is to reduce displacement
    State handle_freebody(const InAir &, const EventHandler &) const;

    State handle_tracker(const OnSegment &, const EventHandler &) const;

    FrameTimeLinkContainer m_frametime_link_container;
};

} // end of point_and_plane namespace
