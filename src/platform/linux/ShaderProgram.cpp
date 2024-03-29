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

#include "ShaderProgram.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <ariajanke/cul/Util.hpp>

#include <array>
#include <sstream>
#include <fstream>

namespace {

constexpr const int k_info_log_size = 512;
using InfoLog = std::array<char, k_info_log_size>;

class ScopedShader {
public:
    static constexpr const unsigned k_no_shader = 0;

    ScopedShader() {}

    ScopedShader(const ScopedShader &) = delete;

    ScopedShader(ScopedShader &&) = delete;

    ~ScopedShader();

    ScopedShader & operator = (const ScopedShader &) = delete;

    ScopedShader & operator = (ScopedShader &&) = delete;

    void compile_vertex(const char * source_code);

    void compile_fragment(const char * source_code);

    unsigned handle() const { return m_shad_handle; }

private:
    void compile(const char * source_code, const char * shader_type);

    unsigned m_shad_handle = k_no_shader;
};

constexpr const char * k_fragment_shader_source =
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"in vec4 vertex_color;\n"
"in vec2 tex_coord;\n"
"\n"
"uniform vec4 our_color;\n"
"uniform sampler2D our_texture;\n"
"\n"
"void main() {\n"
"   FragColor.rgb = texture(our_texture, tex_coord).rgb;\n"
"   FragColor.a = vertex_color.a;\n"
"}\n\0";

constexpr const char * k_vertex_shader_source =
"#version 330 core\n"
"layout (location = 0) in vec3 a_pos;\n"
"layout (location = 1) in vec3 a_color;\n"
"layout (location = 2) in vec2 a_tex_coord;\n"
"\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"uniform vec2 tex_offset;\n"
"uniform float tex_alpha;\n"
"\n"
"out vec4 vertex_color;\n" // specify a color output to the fragment shader
"out vec2 tex_coord;\n"
"\n"
"void main() {\n"
"    gl_Position = projection * view * model * vec4(a_pos, 1.0);\n"
"    vertex_color = vec4(a_color, tex_alpha);\n"
"    tex_coord = a_tex_coord + tex_offset;\n"
"}\n\0";

std::string throwing_file_to_string(const char * filename);

} // end of <anonymous> namespace

ShaderProgram::ShaderProgram(ShaderProgram && lhs)
    { swap(std::move(lhs)); }

ShaderProgram::~ShaderProgram()
    { glDeleteProgram(m_program_handle); }

ShaderProgram & ShaderProgram::operator = (ShaderProgram && lhs) {
    swap(std::move(lhs));
    return *this;
}

void ShaderProgram::load_builtin()
    { load_from_source(k_vertex_shader_source, k_fragment_shader_source); }

void ShaderProgram::load_from_source
    (const char * vertex_shader_source, const char * fragment_shader_source)
{
    ScopedShader vertex_shader, fragment_shader;
    vertex_shader  .compile_vertex  (vertex_shader_source  );
    fragment_shader.compile_fragment(fragment_shader_source);

    // link shaders
    unsigned shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader.handle());
    glAttachShader(shader_program, fragment_shader.handle());
    glLinkProgram(shader_program);

    // check for linking errors
    int success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        InfoLog info_log;
        glGetProgramInfoLog(shader_program, info_log.size(), nullptr, info_log.data());
        throw RuntimeError
            {std::string{"Failed to link shader program:\n"} + info_log.data()};
    }

    m_program_handle = shader_program;
    // note: no explicit "glDeleteShader" calls, destructors do that for us
}

void ShaderProgram::load_from_files
    (const char * vertex_shader_file, const char * fragment_shader_file)
{
    std::string vertex_source   = throwing_file_to_string(vertex_shader_file  );
    std::string fragment_source = throwing_file_to_string(fragment_shader_file);

    load_from_source(vertex_source.c_str(), fragment_source.c_str());
}

void ShaderProgram::use()
    { glUseProgram(m_program_handle); }

void ShaderProgram::set_bool(const char * name, bool val) const
    { glUniform1i(glGetUniformLocation(m_program_handle, name), int(val)); }

void ShaderProgram::set_integer(const char * name, int val) const
    { glUniform1i(glGetUniformLocation(m_program_handle, name), val); }

void ShaderProgram::set_float(const char * name, float val) const
    { glUniform1f(glGetUniformLocation(m_program_handle, name), val); }

void ShaderProgram::set_mat4(const char * name, const glm::mat4 & val) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program_handle, name),
                       1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::set_vec2(const char * name, const glm::vec2 & r) const
    { glUniform2f(glGetUniformLocation(m_program_handle, name), r.x, r.y); }

void ShaderProgram::swap(ShaderProgram && lhs)
    { std::swap(m_program_handle, lhs.m_program_handle); }

Optional<std::string> file_to_string(const char * filename) {
    try {
        return throwing_file_to_string(filename);
    } catch (std::ios_base::failure &) {
        return {};
    }
}

namespace {

ScopedShader::~ScopedShader() {
    // it's worth noting that the shader is not immediately deleted
    // if it's attached to a program object
    glDeleteShader(m_shad_handle);
}

void ScopedShader::compile_vertex(const char * source_code) {
    m_shad_handle = glCreateShader(GL_VERTEX_SHADER);
    compile(source_code, "vertex");
}

void ScopedShader::compile_fragment(const char * source_code) {
    m_shad_handle = glCreateShader(GL_FRAGMENT_SHADER);
    compile(source_code, "fragment");
}

/* private */ void ScopedShader::compile
    (const char * source_code, const char * shader_type)
{
    glShaderSource(m_shad_handle, 1, &source_code, nullptr);
    glCompileShader(m_shad_handle);

    int success;
    glGetShaderiv(m_shad_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        InfoLog info_log;
        glGetShaderInfoLog(m_shad_handle, info_log.size(), nullptr, info_log.data());
        throw RuntimeError
            {"Failed to compile " + std::string{shader_type} + " shader:\n" +
             info_log.data()};
    }
}

std::string throwing_file_to_string(const char * filename) {
    std::ifstream fin;
    fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fin.open(filename);
    std::stringstream sstrm;
    sstrm << fin.rdbuf();
    fin.close();
    return sstrm.str();
}

} // end of <anonymous> namespace
