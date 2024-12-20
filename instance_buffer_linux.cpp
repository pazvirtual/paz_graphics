#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"
#include "util_linux.hpp"
#include "internal_data.hpp"
#include "common.hpp"
#include "gl_core_4_1.h"
#include <numeric>

paz::InstanceBuffer::Data::~Data()
{
    for(auto& n : _ids)
    {
        glDeleteBuffers(1, &n);
    }
    glDeleteVertexArrays(1, &_id);
}

paz::InstanceBuffer::Data::Data()
{
    glGenVertexArrays(1, &_id);
}

paz::InstanceBuffer::InstanceBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::InstanceBuffer::InstanceBuffer(std::size_t size) : InstanceBuffer()
{
    _data->_numInstances = size;
}

void paz::InstanceBuffer::Data::checkSize(int dim, std::size_t size)
{
    const std::size_t m = size/dim;
    if(!_numInstances)
    {
        _numInstances = m;
    }
    else if(m != _numInstances)
    {
        throw std::runtime_error("Number of instances for each attribute must m"
            "atch.");
    }
}

void paz::InstanceBuffer::Data::addAttribute(int dim, DataType type)
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
        default: throw std::logic_error("Instance attribute dimensions must be "
            "1, 2, or 4.");
    }
}

void paz::InstanceBuffer::addAttribute(int dim, DataType type)
{
    if(!_data->_numInstances)
    {
        throw std::runtime_error("Instance buffer size has not been set.");
    }
    _data->addAttribute(dim, type);
    std::size_t s = dim*_data->_numInstances;
    switch(type)
    {
        case DataType::SInt: s *= sizeof(int); break;
        case DataType::UInt: s *= sizeof(unsigned int); break;
        case DataType::Float: s *= sizeof(float); break;
        default: throw std::logic_error("Invalid data type.");
    }
    glBufferData(GL_ARRAY_BUFFER, s, nullptr, GL_DYNAMIC_DRAW);
}

void paz::InstanceBuffer::addAttribute(int dim, const GLfloat* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::Float);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*size, data, GL_STATIC_DRAW);
}

void paz::InstanceBuffer::addAttribute(int dim, const GLuint* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::UInt);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*size, data, GL_STATIC_DRAW);
}

void paz::InstanceBuffer::addAttribute(int dim, const GLint* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->addAttribute(dim, DataType::SInt);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLint)*size, data, GL_STATIC_DRAW);
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLfloat* data,
    std::size_t size)
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

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLuint* data,
    std::size_t size)
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

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLint* data, std::
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

bool paz::InstanceBuffer::empty() const
{
    return !_data || !_data->_numInstances;
}

std::size_t paz::InstanceBuffer::size() const
{
    return _data ? _data->_numInstances : 0;
}

#endif
