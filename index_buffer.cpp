#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::IndexBuffer::Data::~Data()
{
    glDeleteBuffers(1, &_id);
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    glGenBuffers(1, &_data->_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, data,
        GL_STATIC_DRAW);
}

#endif
