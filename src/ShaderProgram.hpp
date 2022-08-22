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

*****************************************************************************/

#pragma once

#include <glm/matrix.hpp>

class ShaderProgram {
public:
    static constexpr const unsigned k_no_program = 0;

    ShaderProgram() {}

    ShaderProgram(const ShaderProgram &) = delete;

    ShaderProgram(ShaderProgram &&);

    ~ShaderProgram();

    ShaderProgram & operator = (const ShaderProgram &) = delete;

    ShaderProgram & operator = (ShaderProgram &&);

    void load_from_source(const char * vertex_shader_source,
                          const char * fragment_shader_source);

    void load_from_files(const char * vertex_shader_file,
                         const char * fragment_shader_file);

    void set_bool   (const char * name, bool ) const;

    void set_integer(const char * name, int  ) const;

    void set_float  (const char * name, float) const;

    void set_mat4   (const char * name, const glm::mat4 &) const;

    void set_vec2   (const char * name, const glm::vec2 &) const;

    void swap(ShaderProgram &&);

    void use();

private:
    unsigned m_program_handle = k_no_program;
};

namespace default_shader_positions {

constexpr const unsigned k_pos_attribute     = 0;
constexpr const unsigned k_color_attribute   = 1;
constexpr const unsigned k_texture_attribute = 2;

} // end of default_shader_positions namespace

ShaderProgram load_default_shader();
