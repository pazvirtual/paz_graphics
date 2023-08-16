#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "util.hpp"
#include "opengl2metal.hpp"

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static unsigned int compile_shader(const std::string& src, bool isVertexShader)
{
    // Compile shader.
    unsigned int shader = glCreateShader(isVertexShader ? GL_VERTEX_SHADER :
        GL_FRAGMENT_SHADER);
    const char* srcStr = src.c_str();
    glShaderSource(shader, 1, &srcStr, NULL);
    glCompileShader(shader);

    // Check compilation.
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        std::string errorLog = paz::get_log(shader, false);
        throw std::runtime_error("Failed to compile " + std::string(
            isVertexShader ? "vertex" : "fragment") + " shader:\n" + errorLog);
    }

    return shader;
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary() {}

paz::ShaderFunctionLibrary::~ShaderFunctionLibrary()
{
    for(auto& n : _vertexIds)
    {
        glDeleteShader(n.second);
    }
    for(auto& n : _fragmentIds)
    {
        glDeleteShader(n.second);
    }
}

void paz::ShaderFunctionLibrary::vertex(const std::string& name, const std::
    string& src)
{
    paz::vert2metal(src);
    _vertexIds[name] = compile_shader(src, true);
}

void paz::ShaderFunctionLibrary::fragment(const std::string& name, const std::
    string& src)
{
    paz::frag2metal(src);
    _fragmentIds[name] = compile_shader(src, false);
}

#endif
