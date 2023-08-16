#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::Shader::~Shader()
{
    glDeleteProgram(_data->_id);
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    _data = std::make_unique<Data>();

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

    // Get uniforms.
    GLint n;
    GLsizei bufSiz;
    glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORMS, &n);
    glGetProgramiv(_data->_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSiz);
    for(GLint i = 0; i < n; ++i)
    {
        GLint size;
        GLenum type;
        std::vector<GLchar> buf;
        buf.resize(bufSiz);
        glGetActiveUniform(_data->_id, i, bufSiz, nullptr, &size, &type, buf.
            data());
        const std::string name(buf.data());
        std::size_t end = name.find("[", 0);
        const GLuint location = glGetUniformLocation(_data->_id, name.substr(0,
            end).c_str());
        _data->_uniformIds[name.substr(0, end)] = std::make_tuple(location,
            type, size);
    }
}

#endif
