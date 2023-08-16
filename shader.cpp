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

paz::Shader::Data::~Data()
{
    glDeleteProgram(_id);
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    initialize();

    _data = std::make_shared<Data>();

    if(!vertLibrary._data->_vertexIds.count(vertName))
    {
        throw std::runtime_error("Vertex function \"" + vertName + "\" not foun"
            "d in library.");
    }

    if(!fragLibrary._data->_fragmentIds.count(fragName))
    {
        throw std::runtime_error("Fragment function \"" + fragName + "\" not fo"
            "und in library.");
    }

    // Link shaders.
    _data->_id = glCreateProgram();
    glAttachShader(_data->_id, vertLibrary._data->_vertexIds.at(vertName));
    glAttachShader(_data->_id, fragLibrary._data->_fragmentIds.at(fragName));
    glLinkProgram(_data->_id);

    // Check linking.
    int success;
    glGetProgramiv(_data->_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::string errorLog = get_log(_data->_id, true);
        throw std::runtime_error("Failed to link shader program:\n" + errorLog);
    }

    // Get vertex attributes.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_data->_id, GL_ACTIVE_ATTRIBUTES, &n);
        glGetProgramiv(_data->_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveAttrib(_data->_id, i, bufSiz, nullptr, &size, &type, buf.
                data());
            std::string name(buf.data());
            name = name.substr(0, name.find("[", 0));
            // `gl_VertexID` has no location because it is a built-in attribute.
            if(name == "gl_VertexID")
            {
                continue;
            }
            const GLint location = glGetAttribLocation(_data->_id, name.
                c_str());
            if(location < 0)
            {
                throw std::logic_error("Vertex attribute \"" + name + "\" is no"
                    "t active.");
            }
            if(size >= 0)
            {
                _data->_attribTypes[location] = type;
            }
        }
    }

    // Get uniforms.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORMS, &n);
        glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveUniform(_data->_id, i, bufSiz, nullptr, &size, &type,
                buf.data());
            const std::string name(buf.data());
            std::size_t end = name.find("[", 0);
            const GLuint location = glGetUniformLocation(_data->_id, name.
                substr(0, end).c_str());
            _data->_uniformIds[name.substr(0, end)] = std::make_tuple(location,
                type, size);
        }
    }
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& geomLibrary, const std::string&
    geomName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    initialize();

    _data = std::make_shared<Data>();

    if(!vertLibrary._data->_vertexIds.count(vertName))
    {
        throw std::runtime_error("Vertex function \"" + vertName + "\" not foun"
            "d in library.");
    }

    if(!geomLibrary._data->_geometryIds.count(geomName))
    {
        throw std::runtime_error("Geometry function \"" + geomName + "\" not fo"
            "und in library.");
    }

    if(!fragLibrary._data->_fragmentIds.count(fragName))
    {
        throw std::runtime_error("Fragment function \"" + fragName + "\" not fo"
            "und in library.");
    }

    // Link shaders.
    _data->_id = glCreateProgram();
    glAttachShader(_data->_id, vertLibrary._data->_vertexIds.at(vertName));
    glAttachShader(_data->_id, geomLibrary._data->_geometryIds.at(geomName));
    glAttachShader(_data->_id, fragLibrary._data->_fragmentIds.at(fragName));
    glLinkProgram(_data->_id);

    // Check linking.
    int success;
    glGetProgramiv(_data->_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::string errorLog = get_log(_data->_id, true);
        throw std::runtime_error("Failed to link shader program:\n" + errorLog);
    }

    // Get vertex attributes.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_data->_id, GL_ACTIVE_ATTRIBUTES, &n);
        glGetProgramiv(_data->_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveAttrib(_data->_id, i, bufSiz, nullptr, &size, &type, buf.
                data());
            std::string name(buf.data());
            name = name.substr(0, name.find("[", 0));
            // `gl_VertexID` has no location because it is a built-in attribute.
            if(name == "gl_VertexID")
            {
                continue;
            }
            const GLint location = glGetAttribLocation(_data->_id, name.
                c_str());
            if(location < 0)
            {
                throw std::logic_error("Vertex attribute \"" + name + "\" is no"
                    "t active.");
            }
            if(size >= 0)
            {
                _data->_attribTypes[location] = type;
            }
        }
    }

    // Get uniforms.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORMS, &n);
        glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveUniform(_data->_id, i, bufSiz, nullptr, &size, &type,
                buf.data());
            const std::string name(buf.data());
            std::size_t end = name.find("[", 0);
            const GLuint location = glGetUniformLocation(_data->_id, name.
                substr(0, end).c_str());
            _data->_uniformIds[name.substr(0, end)] = std::make_tuple(location,
                type, size);
        }
    }
}

#endif
