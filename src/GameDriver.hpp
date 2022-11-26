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

#pragma once

#include "Defs.hpp"
#include "platform.hpp"

class GameDriver {
public:
    static UniquePtr<GameDriver> make_instance();

    virtual ~GameDriver() {}

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    virtual void setup(Platform &) = 0;

    virtual void update(Real seconds, Platform &) = 0;
};
