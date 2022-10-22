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

#include "TextureImpl.hpp"

#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>

#include <ariajanke/cul/Util.hpp>

namespace {

using namespace cul::exceptions_abbr;
static constexpr const int k_rgba_channel_count = 4;

void * allocate_memory(std::size_t n) { return STBI_MALLOC(n); }

void copy_memory(void * dest, const void * src, std::size_t n)
    { ::memcpy(dest, src, n); }

} // end of <anonymous> namespace

OpenGlTexture::OpenGlTexture():
    m_pixel_data(nullptr),
    m_width (0),
    m_height(0),
    m_channel_count(0),
    m_has_texture_id(false)
{}

// we must use malloc here, by API for stb image
// resources are allocated/freed using c standard library
OpenGlTexture::OpenGlTexture(const OpenGlTexture & rhs):
    m_pixel_data    (copy_pixels(rhs)),
    m_width         (rhs.m_width),
    m_height        (rhs.m_height),
    m_channel_count (rhs.m_channel_count),
    m_has_texture_id(false)
{
    if (!m_pixel_data) return;
    generate_texture_and_bind_image_data();
}

OpenGlTexture::OpenGlTexture(OpenGlTexture && rhs): Texture() { swap(rhs); }

OpenGlTexture & OpenGlTexture::operator = (const OpenGlTexture & rhs) {
    if (this != &rhs) {
        OpenGlTexture temp(rhs);
        swap(temp);
    }
    return *this;
}

OpenGlTexture & OpenGlTexture::operator = (OpenGlTexture && rhs) {
    if (this != &rhs)
        swap(rhs);
    return *this;
}

OpenGlTexture::~OpenGlTexture() {
    if (m_has_texture_id)
        glDeleteTextures(1, &m_texture_id);
    if (m_pixel_data)
        stbi_image_free(m_pixel_data);
}

bool OpenGlTexture::load_from_file_no_throw(const char * filename) noexcept {
    // load and generate the texture
    m_pixel_data = stbi_load(filename, &m_width, &m_height,
                             &m_channel_count, k_rgba_channel_count);
    if (!m_pixel_data)
        return false;

    generate_texture_and_bind_image_data();
    return true;
}

void OpenGlTexture::load_from_memory
    (int width_, int height_, const void * rgba_pixels)
{
    static constexpr const char * const k_bad_width_height_msg =
        "Texture::load_from_memory: width and height must be non-negative "
        "integers. They must both be positive integers for a texture to be "
        "created.";
    if (width_ < 0 || height_ < 0) {
        throw InvArg{k_bad_width_height_msg};
    }

    // parameters where doing nothing is apporpiate
    if (width_ == 0 || height_ == 0 || !rgba_pixels) return;

    // again, must match with stbi's allocation functions, therefore malloc/memcpy
    auto bytes_allocated = std::size_t(width_*height_*k_rgba_channel_count);
    void * buf = allocate_memory(bytes_allocated);
    if (!buf) throw std::bad_alloc{};

    copy_memory(buf, rgba_pixels, bytes_allocated);

    m_width         = width_;
    m_height        = height_;
    m_channel_count = k_rgba_channel_count;
    m_pixel_data    = reinterpret_cast<unsigned char *>(buf);

    generate_texture_and_bind_image_data();
}

void OpenGlTexture::bind_texture() const
    { glBindTexture(GL_TEXTURE_2D, m_texture_id); }

void OpenGlTexture::swap(OpenGlTexture & rhs) {
    using std::swap;
    swap(m_pixel_data    , rhs.m_pixel_data    );
    swap(m_width         , rhs.m_width         );
    swap(m_height        , rhs.m_height        );
    swap(m_channel_count , rhs.m_channel_count );
    swap(m_texture_id    , rhs.m_texture_id    );
    swap(m_has_texture_id, rhs.m_has_texture_id);
}

/* private */ unsigned OpenGlTexture::size_in_bytes() const
    { return unsigned(m_width*m_height*m_channel_count); }

/* private */ void OpenGlTexture::generate_texture_and_bind_image_data() {
    assert(m_pixel_data);
    if (!m_has_texture_id) {
        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S    , GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T    , GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        m_has_texture_id = true;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, m_pixel_data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

/* private static */ unsigned char * OpenGlTexture::copy_pixels
    (const OpenGlTexture & rhs)
{
    // libc calls for compatibility with stbi library
    auto data = reinterpret_cast<unsigned char *>(allocate_memory(rhs.size_in_bytes()));
    if (!data) throw std::bad_alloc{};
    copy_memory(data, rhs.m_pixel_data, rhs.size_in_bytes());
    return data;
}
