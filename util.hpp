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
    std::pair<GLint, GLint> min_mag_filter(Texture::MinMagFilter minFilter,
        Texture::MinMagFilter magFilter, paz::Texture::MipmapFilter mipmapFilter
        = Texture::MipmapFilter::None);
    GLint internal_format(int c, int b, Texture::DataType t);
    GLenum gl_type(Texture::DataType t);
    std::string get_log(unsigned int id, bool isProgram);
}

#endif

#endif
