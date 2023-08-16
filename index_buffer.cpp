#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &_data->_id);
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    _data = std::make_unique<Data>();

    _numIndices = size;
    glGenBuffers(1, &_data->_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, data,
        GL_STATIC_DRAW);
}

#endif
