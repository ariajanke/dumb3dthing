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

#include "Defs.hpp"

struct Vertex {
    Vertex() {}
    Vertex(Vector pos, Vector2 tx):
        position(pos), texture_position(tx) {}
    Vector  position         = Vector (0, 0, 0);
    Vector2 texture_position = Vector2(0, 0);
};

struct RenderModelData {
    std::vector<Vertex  > vertices;
    std::vector<unsigned> elements;
};

class RenderModel {
public:
    static std::shared_ptr<RenderModel> make_opengl_instance();

    // used in testing
    static std::shared_ptr<RenderModel> make_null_instance();

    virtual ~RenderModel() {}

    void load(const RenderModelData &);

    template <typename T>
    void load(const std::vector<Vertex> & verticies,
              std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, unsigned>, const std::vector<T> &> vec)
    {
        std::vector<unsigned> els;
        els.reserve(vec.size());
        for (auto el : vec) els.push_back(el);
        load(verticies, els);
    }

    void load(const std::vector<Vertex> &, const std::vector<unsigned> & elements);

    void load(const Vertex   * vertex_beg  , const Vertex   * vertex_end,
              const unsigned * elements_beg, const unsigned * elements_end);

    // no transformations -> needs to be done seperately
    virtual void render() const = 0;

    virtual bool is_loaded() const noexcept = 0;

    explicit operator bool () const noexcept { return is_loaded(); }

protected:
    virtual void load_(const Vertex   * vertex_beg  , const Vertex   * vertex_end,
                       const unsigned * elements_beg, const unsigned * elements_end) = 0;
};

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
