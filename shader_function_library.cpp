#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "opengl2metal.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static const std::string ThickLinesGeomSrc = 1 + R"===(
layout(lines) in;
uniform int paz_Width;
uniform int paz_Height;
layout(triangle_strip, max_vertices = 4) out;
in float glLineWidth[];
void main()
{
    vec2 para = (gl_in[1].gl_Position.xy - gl_in[0].gl_Position.xy);
    para /= length(para);
    vec2 perp = vec2(-para.y, para.x);
    para /= vec2(paz_Width, paz_Height);
    perp /= vec2(paz_Width, paz_Height);
    gl_Position = vec4(gl_in[0].gl_Position.xy - glLineWidth[0]*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy - glLineWidth[1]*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[0].gl_Position.xy + glLineWidth[0]*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy + glLineWidth[1]*perp, 0, 1);
    EmitVertex();
    EndPrimitive();
}
)===";

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

paz::ShaderFunctionLibrary::Data::~Data()
{
    for(auto& n : _vertexIds)
    {
        glDeleteShader(n.second);
    }
    for(auto& n : _fragmentIds)
    {
        glDeleteShader(n.second);
    }

    glDeleteShader(_thickLinesId);
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary()
{
    initialize();

    _data = std::make_shared<Data>();

    try
    {
        _data->_thickLinesId = compile_shader(ThickLinesGeomSrc,
            GL_GEOMETRY_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile geometry function: " + std::
            string(e.what()));
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
        vert2metal(src, _data->_thickLines[name]);
        _data->_vertexIds[name] = compile_shader((_data->_thickLines.at(name) ?
            "#define gl_LineWidth glLineWidth\nout float glLineWidth;\n" : "") +
            src, GL_VERTEX_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile vertex function \"" + name +
            "\": " + e.what());
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
