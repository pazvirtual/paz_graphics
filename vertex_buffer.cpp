#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>
#include <numeric>

paz::VertexBuffer::Data::~Data()
{
    for(auto& n : _ids)
    {
        glDeleteBuffers(1, &n);
    }
    glDeleteVertexArrays(1, &_id);
}

paz::VertexBuffer::Data::Data()
{
    glGenVertexArrays(1, &_id);
}

paz::VertexBuffer::VertexBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::VertexBuffer::Data::checkSize(int dim, std::size_t size)
{
    if(dim != 1 && dim != 2 && dim != 4)
    {
        throw std::runtime_error("Vertex attribute dimensions must be 1, 2, or "
            "4.");
    }
    const std::size_t m = size/dim;
    if(!_numVertices)
    {
        _numVertices = m;
        {
            std::vector<unsigned int> idx(_numVertices + 2);
            std::iota(idx.begin(), idx.end(), -1);
            idx[0] = 0;
            idx.back() = _numVertices - 1;
            _lineStripIndices = IndexBuffer(idx);
        }
        {
            std::vector<unsigned int> idx(_numVertices + 3);
            std::iota(idx.begin(), idx.end(), -1);
            idx[0] = _numVertices - 1;
            idx[idx.size() - 2] = 0;
            idx.back() = 1;
            _lineLoopIndices = IndexBuffer(idx);
        }
        {
            std::vector<unsigned int> idx(2*_numVertices);
            for(unsigned int i = 0; i < _numVertices; ++i)
            {
                idx[2*i] = i;
                idx[2*i + 1] = i;
            }
            _thickLinesIndices = IndexBuffer(idx);
        }
    }
    else if(m != _numVertices)
    {
        throw std::runtime_error("Number of vertices for each attribute must ma"
            "tch.");
    }
}

void paz::VertexBuffer::attribute(int dim, const GLfloat* data, std::size_t
    size)
{
    _data->checkSize(dim, size);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribPointer(i, dim, GL_FLOAT, GL_FALSE, 0, nullptr);
    switch(dim)
    {
        case 1: _data->_types.push_back(GL_FLOAT); break;
        case 2: _data->_types.push_back(GL_FLOAT_VEC2); break;
        case 4: _data->_types.push_back(GL_FLOAT_VEC4); break;
        default: throw std::logic_error("Vertex attribute dimensions must be 1,"
            " 2, or 4.");
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*size, data, GL_STATIC_DRAW);
}

void paz::VertexBuffer::attribute(int dim, const GLuint* data, std::size_t size)
{
    _data->checkSize(dim, size);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribIPointer(i, dim, GL_UNSIGNED_INT, 0, nullptr);
    switch(dim)
    {
        case 1: _data->_types.push_back(GL_UNSIGNED_INT); break;
        case 2: _data->_types.push_back(GL_UNSIGNED_INT_VEC2); break;
        case 4: _data->_types.push_back(GL_UNSIGNED_INT_VEC4); break;
        default: throw std::logic_error("Vertex attribute dimensions must be 1,"
            " 2, or 4.");
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*size, data, GL_STATIC_DRAW);
}

void paz::VertexBuffer::attribute(int dim, const GLint* data, std::size_t size)
{
    _data->checkSize(dim, size);

    const std::size_t i = _data->_ids.size();

    glBindVertexArray(_data->_id);
    _data->_ids.emplace_back();
    glGenBuffers(1, &_data->_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids.back());
    glVertexAttribIPointer(i, dim, GL_INT, 0, nullptr);
    switch(dim)
    {
        case 1: _data->_types.push_back(GL_INT); break;
        case 2: _data->_types.push_back(GL_INT_VEC2); break;
        case 4: _data->_types.push_back(GL_INT_VEC4); break;
        default: throw std::logic_error("Vertex attribute dimensions must be 1,"
            " 2, or 4.");
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*size, data, GL_STATIC_DRAW);
}

#endif
