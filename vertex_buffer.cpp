#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#include "vertex_buffer.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::VertexBuffer::~VertexBuffer()
{
    for(auto& n : _data->_ids)
    {
        glDeleteBuffers(1, &n);
    }
    glDeleteVertexArrays(1, &_data->_id);
}

paz::VertexBuffer::VertexBuffer()
{
    _data = std::make_unique<Data>();

    glGenVertexArrays(1, &_data->_id);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<GLfloat>& data)
{
    check_size(dim, _numVertices, data);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribPointer(i, dim, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*dim*_numVertices, data.data(),
        GL_STATIC_DRAW);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<GLuint>& data)
{
    check_size(dim, _numVertices, data);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribIPointer(i, dim, GL_UNSIGNED_INT, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*dim*_numVertices, data.data(),
        GL_STATIC_DRAW);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<GLint>& data)
{
    check_size(dim, _numVertices, data);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribIPointer(i, dim, GL_INT, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*dim*_numVertices, data.data(),
        GL_STATIC_DRAW);
}

#endif
