#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"
#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>

paz::IndexBuffer::Data::~Data()
{
    glDeleteBuffers(1, &_id);
}

paz::IndexBuffer::IndexBuffer()
{
    initialize();
}

paz::IndexBuffer::IndexBuffer(std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    glGenBuffers(1, &_data->_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, nullptr,
        GL_DYNAMIC_DRAW);
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

void paz::IndexBuffer::sub(const unsigned int* data, std::size_t size)
{
    if(size != _data->_numIndices)
    {
        throw std::runtime_error("Number of instances is fixed.");
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_id);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLuint)*size, data);
}

bool paz::IndexBuffer::empty() const
{
    return !_data || !_data->_numIndices;
}

std::size_t paz::IndexBuffer::size() const
{
    return _data ? _data->_numIndices : 0;
}

#endif
