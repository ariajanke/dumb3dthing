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

#include "../../RenderModel.hpp"

class OpenGlRenderModel final : public RenderModel {
public:
    OpenGlRenderModel() {}

    OpenGlRenderModel(const OpenGlRenderModel &) = delete;

    OpenGlRenderModel(OpenGlRenderModel &&);

    OpenGlRenderModel & operator = (const OpenGlRenderModel &) = delete;

    OpenGlRenderModel & operator = (OpenGlRenderModel &&);

    ~OpenGlRenderModel();

    // no transformations -> needs to be done seperately
    void render() const final;

    void swap(OpenGlRenderModel &&);

    explicit operator bool () const noexcept
        { return m_values_initialized; }

private:
    void load_(const Vertex   * vertex_beg  , const Vertex   * vertex_end,
               const unsigned * elements_beg, const unsigned * elements_end) final;

    bool is_loaded() const noexcept final
        { return m_values_initialized; }

    unsigned m_vbo, m_vao, m_ebo, m_index_count;
    bool m_values_initialized = false;
};
