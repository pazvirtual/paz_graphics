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
    glDeleteBuffers(1, &_lineStripId);
    glDeleteBuffers(1, &_lineLoopId);
    glDeleteBuffers(1, &_thickLinesId);
}

paz::IndexBuffer::IndexBuffer() {}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    glGenBuffers(1, &_data->_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*size, data,
        GL_STATIC_DRAW);
    {
        std::vector<unsigned int> idx(size + 2);
        idx[0] = data[0];
        for(unsigned int i = 1; i < size + 1; ++i)
        {
            idx[i] = data[i - 1];
        }
        idx.back() = data[size - 1];
        glGenBuffers(1, &_data->_lineStripId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_lineStripId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*idx.size(), idx.
            data(), GL_STATIC_DRAW);
    }
    {
        std::vector<unsigned int> idx(size + 3);
        idx[0] = data[size - 1];
        for(unsigned int i = 1; i < size + 1; ++i)
        {
            idx[i] = data[i - 1];
        }
        idx[size + 1] = data[0];
        idx.back() = data[1];
        glGenBuffers(1, &_data->_lineLoopId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_lineLoopId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*idx.size(), idx.
            data(), GL_STATIC_DRAW);
    }
    {
        std::vector<unsigned int> idx(2*size);
        for(std::size_t i = 0; i < size; ++i)
        {
            idx[2*i] = data[i];
            idx[2*i + 1] = data[i];
        }
        glGenBuffers(1, &_data->_thickLinesId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _data->_thickLinesId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*idx.size(), idx.
            data(), GL_STATIC_DRAW);
    }
}

bool paz::IndexBuffer::empty() const
{
    return !_data || !_data->_numIndices;
}

#endif
