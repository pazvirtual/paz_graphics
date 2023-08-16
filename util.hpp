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
    std::pair<GLint, GLint> min_mag_filter(TextureBase::MinMagFilter minFilter,
        TextureBase::MinMagFilter magFilter, paz::TextureBase::MipmapFilter
        mipmapFilter = TextureBase::MipmapFilter::None);
    GLint internal_format(int c, int b, TextureBase::DataType t);
    GLenum gl_type(TextureBase::DataType t);
    GLenum gl_type(VertexBuffer::DataType t);
    GLsizeiptr gl_type_size(VertexBuffer::DataType t);
    std::string get_log(unsigned int id, bool isProgram);
}

#endif

#endif
