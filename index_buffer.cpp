#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &_id);
}

paz::IndexBuffer::IndexBuffer(const std::vector<unsigned int>& indices)
{
    _numIndices = indices.size();
    glGenBuffers(1, &_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*indices.size(),
        indices.data(), GL_STATIC_DRAW);
}

#endif
