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

#include "Texture.hpp"

#include <ariajanke/cul/Util.hpp>

#include <string>

void Texture::load_from_file(const char * filename) {
    using namespace cul::exceptions_abbr;
    if (!load_from_file_no_throw(filename)) {
        throw RtError{  std::string{"Texture::load_from_file: Failed to load texture \""}
                      + filename + "\""};
    }
}
