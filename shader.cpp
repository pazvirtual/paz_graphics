#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "util.hpp"

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

#if 1
#define UNUSED_UNIFORM return;
#else
#define UNUSED_UNIFORM throw std::runtime_error("Uniform \"" + name + "\" is n"\
    "ot used in this shader.");
#endif

paz::Shader::~Shader()
{
    glDeleteProgram(_id);
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    // Link shaders.
    _id = glCreateProgram();
    glAttachShader(_id, vertLibrary._vertexIds.at(vertName));
    glAttachShader(_id, fragLibrary._fragmentIds.at(fragName));
    glLinkProgram(_id);

    // Check linking.
    int success;
    glGetProgramiv(_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::string errorLog = get_log(_id, true);
        throw std::runtime_error("Failed to link shader program:\n" + errorLog);
    }

    // Get uniforms.
    GLint n;
    GLsizei bufSiz;
    glGetProgramiv(_id, GL_ACTIVE_UNIFORMS, &n);
    glGetProgramiv(_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSiz);
    for(GLint i = 0; i < n; ++i)
    {
        GLint size;
        GLenum type;
        std::vector<GLchar> buf;
        buf.resize(bufSiz);
        glGetActiveUniform(_id, i, bufSiz, nullptr, &size, &type, buf.data());
        const std::string name(buf.data());
        std::size_t end = name.find("[", 0);
        const GLuint location = glGetUniformLocation(_id, name.substr(0, end).
            c_str());
        _uniformIds[name.substr(0, end)] = std::make_tuple(location, type,
            size);
    }
}

void paz::Shader::use() const
{
    if(!_id)
    {
        throw std::runtime_error("Shader is not initialized.");
    }
    glUseProgram(_id);
}

// _i (also used for samplers)
void paz::Shader::uniform(const std::string& name, GLint x) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform1i(std::get<0>(_uniformIds.at(name)), x);
}
void paz::Shader::uniform(const std::string& name, GLint x, GLint y) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform2i(std::get<0>(_uniformIds.at(name)), x, y);
}
void paz::Shader::uniform(const std::string& name, GLint x, GLint y, GLint z)
    const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform3i(std::get<0>(_uniformIds.at(name)), x, y, z);
}
void paz::Shader::uniform(const std::string& name, GLint x, GLint y, GLint z,
    GLint w) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform4i(std::get<0>(_uniformIds.at(name)), x, y, z, w);
}

// _iv
void paz::Shader::uniform(const std::string& name, const GLint* x, GLsizei n)
    const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }
    switch(std::get<1>(_uniformIds.at(name)))
    {
        case GL_INT:
            glUniform1iv(std::get<0>(_uniformIds.at(name)), n, x);
            break;
        case GL_INT_VEC2:
            glUniform2iv(std::get<0>(_uniformIds.at(name)), n/2, x);
            break;
        case GL_INT_VEC3:
            glUniform3iv(std::get<0>(_uniformIds.at(name)), n/3, x);
            break;
        case GL_INT_VEC4:
            glUniform4iv(std::get<0>(_uniformIds.at(name)), n/4, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_uniformIds.at(name))) + " for uniform \"" + name +
                "\".");
            break;
    }
}

// _u
void paz::Shader::uniform(const std::string& name, GLuint x) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform1ui(std::get<0>(_uniformIds.at(name)), x);
}
void paz::Shader::uniform(const std::string& name, GLuint x, GLuint y) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform2ui(std::get<0>(_uniformIds.at(name)), x, y);
}
void paz::Shader::uniform(const std::string& name, GLuint x, GLuint y, GLuint z)
    const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform3ui(std::get<0>(_uniformIds.at(name)), x, y, z);
}
void paz::Shader::uniform(const std::string& name, GLuint x, GLuint y, GLuint z,
    GLuint w) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform4ui(std::get<0>(_uniformIds.at(name)), x, y, z, w);
}

// _uiv
void paz::Shader::uniform(const std::string& name, const GLuint* x, GLsizei n)
    const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }
    switch(std::get<1>(_uniformIds.at(name)))
    {
        case GL_UNSIGNED_INT:
            glUniform1uiv(std::get<0>(_uniformIds.at(name)), n, x);
            break;
        case GL_UNSIGNED_INT_VEC2:
            glUniform2uiv(std::get<0>(_uniformIds.at(name)), n/2, x);
            break;
        case GL_UNSIGNED_INT_VEC3:
            glUniform3uiv(std::get<0>(_uniformIds.at(name)), n/3, x);
            break;
        case GL_UNSIGNED_INT_VEC4:
            glUniform4uiv(std::get<0>(_uniformIds.at(name)), n/4, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_uniformIds.at(name))) + " for uniform \"" + name +
                "\".");
            break;
    }
}

// _f
void paz::Shader::uniform(const std::string& name, GLfloat x) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform1f(std::get<0>(_uniformIds.at(name)), x);
}
void paz::Shader::uniform(const std::string& name, GLfloat x, GLfloat y) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform2f(std::get<0>(_uniformIds.at(name)), x, y);
}
void paz::Shader::uniform(const std::string& name, GLfloat x, GLfloat y, GLfloat
    z) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform3f(std::get<0>(_uniformIds.at(name)), x, y, z);
}
void paz::Shader::uniform(const std::string& name, GLfloat x, GLfloat y, GLfloat
    z, GLfloat w) const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }

    glUniform4f(std::get<0>(_uniformIds.at(name)), x, y, z, w);
}

// _fv and _Matrix_fv
void paz::Shader::uniform(const std::string& name, const GLfloat* x, GLsizei n)
    const
{
    if(!_uniformIds.count(name))
    {
        UNUSED_UNIFORM
    }
    switch(std::get<1>(_uniformIds.at(name)))
    {
        case GL_FLOAT:
            glUniform1fv(std::get<0>(_uniformIds.at(name)), n, x);
            break;
        case GL_FLOAT_VEC2:
            glUniform2fv(std::get<0>(_uniformIds.at(name)), n/2, x);
            break;
        case GL_FLOAT_VEC3:
            glUniform3fv(std::get<0>(_uniformIds.at(name)), n/3, x);
            break;
        case GL_FLOAT_VEC4:
            glUniform4fv(std::get<0>(_uniformIds.at(name)), n/4, x);
            break;
        case GL_FLOAT_MAT2:
            glUniformMatrix2fv(std::get<0>(_uniformIds.at(name)), n/4, GL_FALSE,
                x);
            break;
        case GL_FLOAT_MAT3:
            glUniformMatrix3fv(std::get<0>(_uniformIds.at(name)), n/9,
                GL_FALSE, x);
            break;
        case GL_FLOAT_MAT4:
            glUniformMatrix4fv(std::get<0>(_uniformIds.at(name)), n/16,
                GL_FALSE, x);
            break;
        default:
            throw std::invalid_argument("Unsupported type " + std::to_string(
                std::get<1>(_uniformIds.at(name))) + " for uniform \"" + name +
                "\".");
            break;
    }
}

#endif
