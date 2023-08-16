#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include "gl_core_4_1.h"
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

paz::VertexBuffer::VertexBuffer(std::size_t size) : VertexBuffer()
{
    _data->_numVertices = size;
}

void paz::VertexBuffer::Data::checkSize(int dim, std::size_t size)
{
    const std::size_t m = size/dim;
    if(!_numVertices)
    {
        _numVertices = m;
    }
    else if(m != _numVertices)
    {
        throw std::runtime_error("Number of vertices for each attribute must ma"
            "tch.");
    }
}

void paz::VertexBuffer::Data::addAttribute(int dim, DataType type)
{
    const std::size_t i = _ids.size();
    _dims.push_back(dim);
    glBindVertexArray(_id);
    _ids.emplace_back();
    glGenBuffers(1, &_ids.back());
    glEnableVertexAttribArray(i);
    glBindBuffer(GL_ARRAY_BUFFER, _ids.back());
    if(type == DataType::Float)
    {
        glVertexAttribPointer(i, dim, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
    else
    {
        glVertexAttribIPointer(i, dim, gl_type(type), 0, nullptr);
    }
    switch(dim)
    {
        case 1:
            switch(type)
            {
                case DataType::SInt: _types.push_back(GL_INT); break;
                case DataType::UInt: _types.push_back(GL_UNSIGNED_INT); break;
                case DataType::Float: _types.push_back(GL_FLOAT); break;
                default: throw std::logic_error("Invalid data type.");
            }
            break;
        case 2:
            switch(type)
            {
                case DataType::SInt: _types.push_back(GL_INT_VEC2); break;
                case DataType::UInt: _types.push_back(GL_UNSIGNED_INT_VEC2);
                    break;
                case DataType::Float: _types.push_back(GL_FLOAT_VEC2); break;
                default: throw std::logic_error("Invalid data type.");
            }
            break;
        case 4:
            switch(type)
            {
                case DataType::SInt: _types.push_back(GL_INT_VEC4); break;
                case DataType::UInt: _types.push_back(GL_UNSIGNED_INT_VEC4);
                    break;
                case DataType::Float: _types.push_back(GL_FLOAT_VEC4); break;
                default: throw std::logic_error("Invalid data type.");
            }
            break;
        default: throw std::logic_error("Vertex attribute dimensions must be 1,"
            " 2, or 4.");
    }
}

void paz::VertexBuffer::addAttribute(int dim, DataType type)
{
    if(!_data->_numVertices)
    {
        throw std::runtime_error("Vertex buffer size has not been set.");
    }
    _data->addAttribute(dim, type);
    std::size_t s = dim*_data->_numVertices;
    switch(type)
    {
        case DataType::SInt: s *= sizeof(int); break;
        case DataType::UInt: s *= sizeof(unsigned int); break;
        case DataType::Float: s *= sizeof(float); break;
        default: throw std::logic_error("Invalid data type.");
    }
    glBufferData(GL_ARRAY_BUFFER, s, nullptr, GL_DYNAMIC_DRAW);
}

void paz::VertexBuffer::addAttribute(int dim, const GLfloat* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::Float);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*size, data, GL_STATIC_DRAW);
}

void paz::VertexBuffer::addAttribute(int dim, const GLuint* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::UInt);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*size, data, GL_STATIC_DRAW);
}

void paz::VertexBuffer::addAttribute(int dim, const GLint* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::SInt);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*size, data, GL_STATIC_DRAW);
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLfloat* data, std::
    size_t size)
{
    if(idx >= _data->_ids.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    if(_data->_types[idx] != GL_FLOAT && _data->_types[idx] != GL_FLOAT_VEC2 &&
        _data->_types[idx] != GL_FLOAT_VEC4)
    {
        throw std::logic_error("Attribute type does not match.");
    }
    _data->checkSize(_data->_dims[idx], size);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids[idx]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*size, data);
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLuint* data, std::
    size_t size)
{
    if(idx >= _data->_ids.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    if(_data->_types[idx] != GL_UNSIGNED_INT && _data->_types[idx] !=
        GL_UNSIGNED_INT_VEC2 && _data->_types[idx] != GL_UNSIGNED_INT_VEC4)
    {
        throw std::logic_error("Attribute type does not match.");
    }
    _data->checkSize(_data->_dims[idx], size);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids[idx]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLuint)*size, data);
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLint* data, std::
    size_t size)
{
    if(idx >= _data->_ids.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    if(_data->_types[idx] != GL_INT && _data->_types[idx] != GL_INT_VEC2 &&
        _data->_types[idx] != GL_INT_VEC4)
    {
        throw std::logic_error("Attribute type does not match.");
    }
    _data->checkSize(_data->_dims[idx], size);
    glBindBuffer(GL_ARRAY_BUFFER, _data->_ids[idx]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLint)*size, data);
}

bool paz::VertexBuffer::empty() const
{
    return !_data || !_data->_numVertices;
}

std::size_t paz::VertexBuffer::size() const
{
    return _data ? _data->_numVertices : 0;
}

#endif
