#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

template<typename T>
static void check_size(int dim, std::size_t& n, const std::vector<T>& d)
{
    if(dim != 1 && dim != 2 && dim != 4)
    {
        throw std::runtime_error("Vertex attribute dimensions must be 1, 2, or "
            "4.");
    }
    const std::size_t m = d.size()/dim;
    if(!n)
    {
        n = m;
    }
    else if(m != n)
    {
        throw std::runtime_error("Number of vertices for each attribute must ma"
            "tch.");
    }
}

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

void paz::VertexBuffer::attribute(int dim, const std::vector<float>& data)
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

void paz::VertexBuffer::attribute(int dim, const std::vector<unsigned int>&
    data)
{
    check_size(dim, _numVertices, data);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribPointer(i, dim, GL_UNSIGNED_INT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*dim*_numVertices, data.data(),
        GL_STATIC_DRAW);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<int>& data)
{
    check_size(dim, _numVertices, data);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribPointer(i, dim, GL_INT, GL_FALSE, 0, nullptr);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*dim*_numVertices, data.data(),
        GL_STATIC_DRAW);
}

#endif
