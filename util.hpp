#ifndef UTIL_HPP
#define UTIL_HPP

#include "PAZ_Graphics"

#ifndef PAZ_MACOS

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
    GLenum gl_type(VertexBuffer::DataType t);
    GLsizeiptr gl_type_size(VertexBuffer::DataType t);
    std::string get_log(unsigned int id, bool isProgram);
}

#endif

#endif
