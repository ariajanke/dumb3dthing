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

#include "RenderModelImpl.hpp"
#include "ShaderProgram.hpp"
#include "GlmDefs.hpp"

#include <glad/glad.h>

OpenGlRenderModel::OpenGlRenderModel(OpenGlRenderModel && lhs)
    { swap(std::move(lhs)); }

OpenGlRenderModel & OpenGlRenderModel::operator = (OpenGlRenderModel && lhs) {
    swap(std::move(lhs));
    return *this;
}

OpenGlRenderModel::~OpenGlRenderModel() {
    if (!m_values_initialized) return;
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers     (1, &m_ebo);
    glDeleteBuffers     (1, &m_vbo);
}

// no transformations -> needs to be done seperately
void OpenGlRenderModel::render() const {
    assert(m_values_initialized);
    // note, you could render a different set of elements
    // I happen to be simplifying things quite a bit for this class
    // maybe in the future weird/interesting things can be done in a new class
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, int(m_index_count), GL_UNSIGNED_INT, nullptr);
}

void OpenGlRenderModel::swap(OpenGlRenderModel && lhs) {
    using std::swap;
    swap(m_vbo               , lhs.m_vbo               );
    swap(m_vao               , lhs.m_vao               );
    swap(m_ebo               , lhs.m_ebo               );
    swap(m_index_count       , lhs.m_index_count       );
    swap(m_values_initialized, lhs.m_values_initialized);
}

/* private */ void OpenGlRenderModel::load_
    (const Vertex   * vertex_beg  , const Vertex   * vertex_end,
     const unsigned * elements_beg, const unsigned * elements_end)
{
    std::vector<float> vertex_data;
    vertex_data.reserve(8*(vertex_end - vertex_beg));
    for (const auto & vert : cul::View{vertex_beg, vertex_end}) {
        vertex_data.push_back(float(vert.position.x));
        vertex_data.push_back(float(vert.position.y));
        vertex_data.push_back(float(vert.position.z));

        // colors
        // maybe on the shader level we should omit colors?
        vertex_data.push_back(1.f);
        vertex_data.push_back(1.f);
        vertex_data.push_back(1.f);

        vertex_data.push_back(float(vert.texture_position.x));
        vertex_data.push_back(float(vert.texture_position.y));
    }

    unsigned int vbo, vao, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s),
    // and then configure vertex attributes(s).
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertex_data.size()*sizeof(float)),
                 &vertex_data.front(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>((elements_end - elements_beg)*sizeof(unsigned)),
                 elements_beg, GL_STATIC_DRAW);
    // this needs to be synced up to the shader
    // if the shader inputs changes, then this also needs to change
    // specifically: the attribute positions
    using namespace default_shader_positions;
    constexpr const unsigned k_vertex_size = 8;
    glVertexAttribPointer(k_pos_attribute, 3, GL_FLOAT, GL_FALSE,
                          k_vertex_size * sizeof(float),
                          pointer_offset(0)  );
    glEnableVertexAttribArray(k_pos_attribute);

    glVertexAttribPointer(k_color_attribute, 3, GL_FLOAT, GL_FALSE,
                          k_vertex_size * sizeof(float),
                          pointer_offset( (3)*sizeof(float) ) );
    glEnableVertexAttribArray(k_color_attribute);

    glVertexAttribPointer(k_texture_attribute, 2, GL_FLOAT, GL_FALSE,
                          k_vertex_size * sizeof(float),
                          pointer_offset( (3 + 3)*sizeof(float) ) );
    glEnableVertexAttribArray(k_texture_attribute);

    // note that this is allowed, the call to glVertexAttribPointer registered
    // VBO as the vertex attribute's bound vertex buffer object so afterwards
    // we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally
    // modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't
    // unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    m_vbo = vbo;
    m_ebo = ebo;
    m_vao = vao;
    m_index_count = unsigned(elements_end - elements_beg);
    m_values_initialized = true;
}
