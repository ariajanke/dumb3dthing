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

#pragma once

#include <memory>

class Texture {
public:
    static std::shared_ptr<Texture> make_opengl_instance();

    static std::shared_ptr<Texture> make_null_instance();

    virtual ~Texture() {}

    virtual bool load_from_file_no_throw(const char *) noexcept = 0;

    void load_from_file(const char *);

    virtual void load_from_memory(int width_, int height_, const void * rgba_pixels) = 0;

    virtual int width () const = 0;

    virtual int height() const = 0;

    virtual void bind_texture(/* there is a rendering context in WebGL */) const = 0;
};

class OpenGlTexture final : public Texture {
public:
    OpenGlTexture();

    OpenGlTexture(const OpenGlTexture &);

    OpenGlTexture(OpenGlTexture &&);

    ~OpenGlTexture() final;

    OpenGlTexture & operator = (const OpenGlTexture &);

    OpenGlTexture & operator = (OpenGlTexture &&);

    bool load_from_file_no_throw(const char *) noexcept final;

    void load_from_memory(int width_, int height_, const void * rgba_pixels) final;

    int width () const final { return m_width ; }

    int height() const final { return m_height; }

    void bind_texture(/* there is a rendering context in WebGL */) const final;

    void swap(OpenGlTexture &);

private:
    unsigned size_in_bytes() const;

    void generate_texture_and_bind_image_data();

    static unsigned char * copy_pixels(const OpenGlTexture &);

    unsigned char * m_pixel_data;
    int m_width, m_height;
    int m_channel_count;
    unsigned m_texture_id;
    bool m_has_texture_id;
};
