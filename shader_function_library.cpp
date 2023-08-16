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

paz::ShaderFunctionLibrary::ShaderFunctionLibrary(const std::vector<std::pair<
    std::string, std::string>>& vertSrcs, const std::vector<std::pair<std::
    string, std::string>>& fragSrcs)
{
    for(const auto& n : vertSrcs)
    {
        paz::vert2metal(n.second);
        _vertexIds[n.first] = compile_shader(n.second, true);
    }
    for(const auto& n : fragSrcs)
    {
        paz::frag2metal(n.second);
        _fragmentIds[n.first] = compile_shader(n.second, false);
    }
}

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

#endif
