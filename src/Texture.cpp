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

*******************************************************************************

    This is a derivative work of Joey de Vries' OpenGL tutorials.

    By https://learnopengl.com/#!About, the original works is copyrighted
    by Joey de Vries and is released unde the CC-BY-4.0 international license.

    A human readable version is available here:
    https://creativecommons.org/licenses/by/4.0/

    Full license text is available here:
    https://creativecommons.org/licenses/by/4.0/legalcode

    Since mutliple tutorials were used in the creation of this source file:
    Getting started -> Textures
    https://learnopengl.com/#!Getting-started/Textures

*****************************************************************************/

#include "Texture.hpp"

#include <common/Util.hpp>

void Texture::load_from_file(const char * filename) {
    using namespace cul::exceptions_abbr;
    if (!load_from_file_no_throw(filename)) {
        throw RtError{  std::string{"Texture::load_from_file: Failed to load texture \""}
                      + filename + "\""};
    }
}
