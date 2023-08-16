#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "opengl2metal.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include <sstream>
#include <regex>
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static bool uses_gl_line_width(const std::string& vertSrc)
{
    std::istringstream iss(vertSrc);
    std::string line;
    while(std::getline(iss, line))
    {
        line = std::regex_replace(line, std::regex("//.*$"), "");
        line = std::regex_replace(line, std::regex("\\s+$"), "");
        if(line.empty())
        {
            continue;
        }
        if(line.find("gl_LineWidth") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

std::string prep_for_geom(const std::string& vertSrc, std::string& block, std::
    string& block0, std::string& block1)
{
    std::unordered_map<std::string, std::pair<std::string, bool>> outputs;
    {
        std::istringstream iss(vertSrc);
        std::string line;
        while(std::getline(iss, line))
        {
            line = std::regex_replace(line, std::regex("//.*$"), "");
            line = std::regex_replace(line, std::regex("\\s+$"), "");
            if(line.empty())
            {
                continue;
            }
            if(std::regex_match(line, std::regex("out\\s.*")))
            {
                const std::string dec = line.substr(4, line.size() - 5);
                const std::size_t pos = dec.find_last_of(' ');
                const std::string type = dec.substr(0, pos);
                const std::string name = dec.substr(pos + 1);
                outputs[name] = {type, false};
                continue;
            }
            if(std::regex_match(line, std::regex("flat out\\s.*")))
            {
                const std::string dec = line.substr(9, line.size() - 10);
                const std::size_t pos = dec.find_last_of(' ');
                const std::string type = dec.substr(0, pos);
                const std::string name = dec.substr(pos + 1);
                outputs[name] = {type, true};
                continue;
            }
        }
    }
    if(outputs.empty())
    {
        return vertSrc;
    }

    std::string regexStr;
    for(const auto& n : outputs)
    {
        regexStr += "|" + n.first;
        block += std::string(n.second.second ? "flat " : "") + "in " + n.
            second.first + " inout_" + n.first + "[];\n" + (n.second.second ?
            "flat " : "") + "out " + n.second.first + " " + n.first + ";\n";
        block0 += n.first + " = inout_" + n.first + "[0];\n";
        block1 += n.first + " = inout_" + n.first + "[1];\n";
    }
    regexStr = "\\b(" + regexStr.substr(1) + ")\\b";

    std::string modifiedSrc;
    std::istringstream iss(vertSrc);
    std::string line;
    while(std::getline(iss, line))
    {
        line = std::regex_replace(line, std::regex(regexStr), "inout_$1");
        modifiedSrc += line + "\n";
    }

    return modifiedSrc;
}

static const std::string Src0 = 1 + R"===(
layout(lines) in;
uniform int paz_Width;
uniform int paz_Height;
layout(triangle_strip, max_vertices = 4) out;
in float glLineWidth[];
)===";

static const std::string Src1 = 1 + R"===(
void main()
{
    vec2 size = vec2(paz_Width, paz_Height);
    vec2 para = normalize((gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w -
        gl_in[0].gl_Position.xy/gl_in[1].gl_Position.w)*size);
    vec2 perp = vec2(-para.y, para.x)/size;
    float s0 = glLineWidth[0]*gl_in[0].gl_Position.w;
    float s1 = glLineWidth[1]*gl_in[1].gl_Position.w;
    gl_Position = vec4(gl_in[0].gl_Position.xy - s0*perp, gl_in[0].
        gl_Position.zw);
)===";

static const std::string Src2 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy - s1*perp, gl_in[0].
        gl_Position.zw);
)===";

static const std::string Src3 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[0].gl_Position.xy + s0*perp, gl_in[0].
        gl_Position.zw);
)===";

static const std::string Src4 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy + s1*perp, gl_in[0].
        gl_Position.zw);
)===";

static const std::string Src5 = 1 + R"===(
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
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary()
{
    initialize();

    _data = std::make_shared<Data>();
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
        const bool usesGlLineWidth = uses_gl_line_width(src);
        if(usesGlLineWidth)
        {
            std::string block;
            std::string block0;
            std::string block1;
            const std::string modifiedSrc = prep_for_geom(src, block, block0,
                block1);
            _data->_vertexIds[name] = compile_shader("#define gl_LineWidth glLi"
                "neWidth\nout float glLineWidth;\n" + modifiedSrc,
                GL_VERTEX_SHADER);
            _data->_thickLinesIds[name] = compile_shader(Src0 + block + Src1 +
                block0 + Src2 + block1 + Src3 + block0 + Src4 + block1 + Src5,
                GL_GEOMETRY_SHADER);
        }
        else
        {
            _data->_vertexIds[name] = compile_shader(src, GL_VERTEX_SHADER);
            _data->_thickLinesIds[name] = 0;
        }
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
