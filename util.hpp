#ifndef PAZ_GRAPHICS_UTIL_HPP
#define PAZ_GRAPHICS_UTIL_HPP

#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

namespace paz
{
    constexpr int GlMajorVersion = 4;
    constexpr int GlMinorVersion = 1;

    std::pair<GLint, GLint> min_mag_filter(Texture::MinMagFilter minFilter,
        Texture::MinMagFilter magFilter, paz::Texture::MipmapFilter mipmapFilter
        = Texture::MipmapFilter::None);
    GLint gl_internal_format(Texture::Format format);
    GLenum gl_format(Texture::Format format);
    GLenum gl_type(Texture::Format format);
    std::string get_log(unsigned int id, bool isProgram);
    std::string gl_error(GLenum error);
}

#endif

#endif
