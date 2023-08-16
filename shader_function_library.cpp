#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "opengl2metal.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static unsigned int compile_shader(const std::string& src, GLenum type)
{
    // Compile shader.
    unsigned int shader = glCreateShader(type);
    const std::string headerStr = "#version " + std::to_string(paz::
        GlMajorVersion) + std::to_string(paz::GlMinorVersion) + "0 core\n#defin"
        "e depthSampler2D sampler2D\n";
    std::array<const char*, 2> srcStrs = {headerStr.c_str(), src.c_str()};
    glShaderSource(shader, srcStrs.size(), srcStrs.data(), NULL);
    glCompileShader(shader);

    // Check compilation.
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        std::string errorLog = paz::get_log(shader, false);
        throw std::runtime_error("\n" + errorLog);
    }

    return shader;
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary()
{
    _data = std::make_unique<Data>();
}

paz::ShaderFunctionLibrary::~ShaderFunctionLibrary()
{
    for(auto& n : _data->_vertexIds)
    {
        glDeleteShader(n.second);
    }
    for(auto& n : _data->_fragmentIds)
    {
        glDeleteShader(n.second);
    }
}

void paz::ShaderFunctionLibrary::vertex(const std::string& name, const std::
    string& src)
{
    if(_data->_vertexIds.count(name))
    {
        throw std::runtime_error("Vertex function \"" + name + "\" has already "
            "been defined.");
    }

    try
    {
        vert2metal(src);
        _data->_vertexIds[name] = compile_shader(src, GL_VERTEX_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile vertex function \"" + name +
            "\": " + e.what());
    }
}

void paz::ShaderFunctionLibrary::geometry(const std::string& name, const std::
    string& src)
{
    if(_data->_geometryIds.count(name))
    {
        throw std::runtime_error("Geometry function \"" + name + "\" has alread"
            "y been defined.");
    }

    try
    {
//        geom2metal(src);
        _data->_geometryIds[name] = compile_shader(src, GL_GEOMETRY_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile geometry function \"" + name
            + "\": " + e.what());
    }
}

void paz::ShaderFunctionLibrary::fragment(const std::string& name, const std::
    string& src)
{
    if(_data->_fragmentIds.count(name))
    {
        throw std::runtime_error("Fragment function \"" + name + "\" has alread"
            "y been defined.");
    }

    try
    {
        frag2metal(src);
        _data->_fragmentIds[name] = compile_shader(src, GL_FRAGMENT_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile fragment function \"" + name
            + "\": " + e.what());
    }
}

#endif
