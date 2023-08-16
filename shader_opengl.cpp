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

paz::ShaderData::~ShaderData()
{
    if(_id)
    {
        glDeleteProgram(_id);
    }
}

paz::ShaderData::ShaderData() {}

void paz::ShaderData::init(unsigned int vertId, unsigned int thickLinesVertId,
    unsigned int thickLinesGeomId, unsigned int fragId)
{
    if(_id)
    {
        throw std::logic_error("Shader has already been initialized.");
    }

    initialize();

    // Link shaders.
    _id = glCreateProgram();
    glAttachShader(_id, vertId);
    glAttachShader(_id, fragId);
    glLinkProgram(_id);

    // Check linking.
    int success;
    glGetProgramiv(_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        std::string errorLog = get_log(_id, true);
        throw std::runtime_error("Failed to link shader program:\n" + errorLog);
    }

    // Get vertex attributes.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_id, GL_ACTIVE_ATTRIBUTES, &n);
        glGetProgramiv(_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveAttrib(_id, i, bufSiz, nullptr, &size, &type, buf.
                data());
            std::string name(buf.data());
            name = name.substr(0, name.find("[", 0));
            // `gl_VertexID` has no location because it is a built-in attribute.
            if(name == "gl_VertexID")
            {
                continue;
            }
            const GLint location = glGetAttribLocation(_id, name.c_str());
            if(location < 0)
            {
                throw std::logic_error("Vertex attribute \"" + name + "\" is no"
                    "t active.");
            }
            if(size >= 0)
            {
                _attribTypes[location] = type;
            }
        }
    }

    // Get uniforms.
    {
        GLint n;
        GLsizei bufSiz;
        glGetProgramiv(_id, GL_ACTIVE_UNIFORMS, &n);
        glGetProgramiv(_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSiz);
        for(GLint i = 0; i < n; ++i)
        {
            GLint size;
            GLenum type;
            std::vector<GLchar> buf(bufSiz);
            glGetActiveUniform(_id, i, bufSiz, nullptr, &size, &type, buf.
                data());
            const std::string name(buf.data());
            std::size_t end = name.find("[", 0);
            const GLuint location = glGetUniformLocation(_id, name.substr(0,
                end).c_str());
            _uniformIds[name.substr(0, end)] = std::make_tuple(location, type,
                size);
        }
    }

    // Link thick lines variant.
    if(thickLinesVertId)
    {
        // Link shaders.
        _thickLinesId = glCreateProgram();
        glAttachShader(_thickLinesId, thickLinesVertId);
        glAttachShader(_thickLinesId, thickLinesGeomId);
        glAttachShader(_thickLinesId, fragId);
        glLinkProgram(_thickLinesId);

        // Check linking.
        int success;
        glGetProgramiv(_thickLinesId, GL_LINK_STATUS, &success);
        if(!success)
        {
            std::string errorLog = get_log(_thickLinesId, true);
            throw std::runtime_error("Failed to link thick lines shader program"
                ":\n" + errorLog);
        }

        // Get uniforms.
        {
            GLint n;
            GLsizei bufSiz;
            glGetProgramiv(_thickLinesId, GL_ACTIVE_UNIFORMS, &n);
            glGetProgramiv(_thickLinesId, GL_ACTIVE_UNIFORM_MAX_LENGTH,
                &bufSiz);
            for(GLint i = 0; i < n; ++i)
            {
                GLint size;
                GLenum type;
                std::vector<GLchar> buf(bufSiz);
                glGetActiveUniform(_thickLinesId, i, bufSiz, nullptr, &size,
                    &type, buf.data());
                const std::string name(buf.data());
                std::size_t end = name.find("[", 0);
                const GLuint location = glGetUniformLocation(_thickLinesId,
                    name.substr(0, end).c_str());
                _thickLinesUniformIds[name.substr(0, end)] = std::make_tuple(
                    location, type, size);
            }
        }
    }
}

#endif
