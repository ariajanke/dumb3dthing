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

#include "PlayerControl.hpp"

void PlayerControl::press(KeyControl ky) {
    using Kc = KeyControl;
    switch (ky) {
    case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
    case Kc::camera_left: case Kc::camera_right:
        m_dir[to_index(ky)] = true;
        break;
    case Kc::jump:
        m_jump_this_frame = true;
        break;
    default: return;
    }
}

void PlayerControl::release(KeyControl ky) {
    using Kc = KeyControl;
    switch (ky) {
    case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
    case Kc::camera_left: case Kc::camera_right:
        m_dir[to_index(ky)] = false;
        break;
    case Kc::jump:
        m_jump_this_frame = false;
        break;
    default: return;
    }
}

void PlayerControl::frame_update()
    { m_jump_pressed_before = m_jump_this_frame; }

Vector2 PlayerControl::heading() const {
    using Kc = KeyControl;
    auto left  = m_dir[to_index(Kc::left    )];
    auto right = m_dir[to_index(Kc::right   )];
    auto back  = m_dir[to_index(Kc::backward)];
    auto fore  = m_dir[to_index(Kc::forward )];
    bool x_ways = left ^ right;
    bool y_ways = back ^ fore ;
    if (!x_ways && !y_ways) return Vector2{};
    return normalize(Vector2{
        x_ways*(right - left)*1.,
        y_ways*(fore  - back)*1.});
}

bool PlayerControl::is_starting_jump() const
    { return !m_jump_pressed_before && m_jump_this_frame; }

bool PlayerControl::is_ending_jump() const
    { return m_jump_pressed_before && !m_jump_this_frame; }

Real PlayerControl::camera_rotation_direction() const {
    using Kc = KeyControl;
    return to_direction
        (m_dir[to_index(Kc::camera_left )],
         m_dir[to_index(Kc::camera_right)]);
}

/* private static */ int PlayerControl::to_index(KeyControl ky) {
    using Kc = KeyControl;
    switch (ky) {
    case Kc::forward : return 0; case Kc::backward: return 1;
    case Kc::left    : return 2; case Kc::right   : return 3;
    case Kc::camera_left: return 4; case Kc::camera_right: return 5;
    default: break;
    }
    throw std::runtime_error{""};
}

/* private static */ Real PlayerControl::to_direction(bool neg, bool pos) {
    bool i_ways = neg ^ pos;
    return static_cast<Real>(i_ways)*
        ( static_cast<int>(pos) - static_cast<int>(neg) )*1.;
}
