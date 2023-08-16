#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
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

paz::VertexBuffer::VertexBuffer(const std::vector<InputData>& data)
{
    _data = std::make_unique<Data>();

    _numVertices = (data.empty() ? 0 : data[0]._numVertices);
    glGenVertexArrays(1, &_data->_id);
    glBindVertexArray(_data->_id);
    const std::size_t numAttribs = data.size();
    _data->_ids.resize(numAttribs);
    glGenBuffers(numAttribs, _data->_ids.data());
    for(std::size_t i = 0; i < numAttribs; ++i)
    {
        if(data[i]._numVertices != _numVertices)
        {
            throw std::runtime_error("Number of vertices for each attribute mus"
                "t match.");
        }
        if(data[i]._dim != 1 && data[i]._dim != 2 && data[i]._dim != 4)
        {
            throw std::runtime_error("Vertex attribute dimensions must be 1, 2,"
                " or 4.");
        }
        glEnableVertexAttribArray(i);
        glBindBuffer(GL_ARRAY_BUFFER, _data->_ids[i]);
        glVertexAttribPointer(i, data[i]._dim, gl_type(data[i]._type), GL_FALSE,
            0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, gl_type_size(data[i]._type)*data[i]._dim*
            _numVertices, data[i]._data, GL_STATIC_DRAW);
    }
}

#endif
