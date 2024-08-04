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

#include "platform.hpp"

class PlayerControl final {
public:
    void press(KeyControl);

    void release(KeyControl);

    void frame_update();

    /** @returns either a normal or zero vector */
    Vector2 heading() const;

    bool is_starting_jump() const;

    bool is_ending_jump() const;

    Real camera_rotation_direction() const;

private:
    static int to_index(KeyControl ky);

    static Real to_direction(bool neg, bool pos);

    std::array<bool, 6> m_dir = std::array<bool, 6>{};
    bool m_jump_pressed_before = false;
    bool m_jump_this_frame = false;
};
