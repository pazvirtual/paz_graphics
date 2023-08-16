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

static std::string fix_initializers(const std::string& src)
{
    int inInit = 0;
    std::string curType;
    std::string types;

    std::istringstream in(src);
    std::ostringstream out;
    std::string line;
    while(std::getline(in, line))
    {
        if(std::regex_match(line, std::regex("\\s*struct\\s+[a-zA-Z_][a-zA-Z_0-"
            "9]*\\b.*")))
        {
            types += (types.empty() ? "" : "|") + std::regex_replace(line, std::
                regex("^\\s*struct\\s+([a-zA-Z_][a-zA-Z_0-9]*)\\b"), "$1");
        }
        else if(std::regex_match(line, std::regex(".*\\b(float|u?int|[iu]?vec[2"
            "-4])\\s+[a-zA-Z_][a-zA-Z_0-9]*\\s*\\[.*\\]\\s*=.*")))
        {
            inInit = 2;
            curType = std::regex_replace(line, std::regex(".*\\b(float|u?int|[i"
                "u]?vec[2-4])\\s+[a-zA-Z_][a-zA-Z_0-9]*\\s*\\[.*\\]\\s*=.*"),
                "$1") + "[]";
        }
        else if(!types.empty() && std::regex_match(line, std::regex(".*\\b(cons"
            "t\\s+)?(" + types + ")\\s+[a-zA-Z_][a-zA-Z_0-9]*\\s*=.*")))
        {
            inInit = 2;
            curType = std::regex_replace(line, std::regex(".*\\b(const\\s+)?(" +
                types + ")\\s+[a-zA-Z_][a-zA-Z_0-9]*\\s*=.*"), "$2");
        }

        std::size_t pos = 0;
        if(inInit)
        {
            while(pos < line.size())
            {
                if(inInit == 2 && line[pos] == '{')
                {
                    line[pos] = '(';
                    line.insert(pos, curType);
                    pos += curType.size();
                    inInit = 1;
                }
                else if(inInit == 1 && line[pos] == '}')
                {
                    line[pos] = ')';
                    inInit = 0;
                }
                ++pos;
            }
        }

        out << line << std::endl;
    }

    return out.str();
}

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
        block0 += n.first + " = inout_" + n.first + "[1];\n";
        block1 += n.first + " = inout_" + n.first + "[2];\n";
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
layout(lines_adjacency) in;
uniform int paz_Width;
uniform int paz_Height;
layout(triangle_strip, max_vertices = 4) out;
in float glLineWidth[];
)===";

static const std::string Src1 = 1 + R"===(
#ifndef M_PI
#define M_PI 3.14159265358979323846264338328
#endif
#define PAZ_ANG 0.5*M_PI
void main()
{
    vec2 paz_size = vec2(paz_Width, paz_Height);
    vec2 paz_paraPrev = normalize((gl_in[1].gl_Position.xy/gl_in[1].gl_Position.
        w - gl_in[0].gl_Position.xy/gl_in[0].gl_Position.w)*paz_size);
    vec2 paz_para = normalize((gl_in[2].gl_Position.xy/gl_in[2].gl_Position.w -
        gl_in[1].gl_Position.xy/gl_in[1].gl_Position.w)*paz_size);
    vec2 paz_paraNext = normalize((gl_in[3].gl_Position.xy/gl_in[3].gl_Position.
        w - gl_in[2].gl_Position.xy/gl_in[2].gl_Position.w)*paz_size);
    float paz_angle = atan(paz_para.y, paz_para.x);
    float paz_anglePrev = atan(paz_paraPrev.y, paz_paraPrev.x) - paz_angle;
    float paz_angleNext = atan(paz_paraNext.y, paz_paraNext.x) - paz_angle;
    paz_anglePrev = fract(paz_anglePrev*0.5/M_PI + 0.5)*2.*M_PI - M_PI;
    paz_angleNext = fract(paz_angleNext*0.5/M_PI + 0.5)*2.*M_PI - M_PI;
    float paz_a = sin(0.5*(M_PI - abs(paz_anglePrev)));
    float paz_b = sin(0.5*(M_PI - abs(paz_angleNext)));
    const mat2 paz_r = mat2(cos(PAZ_ANG), -sin(PAZ_ANG), sin(PAZ_ANG), cos(PAZ_ANG));
    const mat2 paz_s = mat2(cos(PAZ_ANG), sin(PAZ_ANG), -sin(PAZ_ANG), cos(PAZ_ANG));
    if(paz_anglePrev > PAZ_ANG)
    {
        paz_paraPrev = paz_s*paz_para;
        paz_anglePrev = PAZ_ANG;
        paz_a = sin(0.5*(M_PI - PAZ_ANG));
    }
    else if(paz_anglePrev < -PAZ_ANG)
    {
        paz_paraPrev = paz_r*paz_para;
        paz_anglePrev = PAZ_ANG;
        paz_a = sin(0.5*(M_PI - PAZ_ANG));
    }
    if(paz_angleNext > PAZ_ANG)
    {
        paz_paraNext = paz_s*paz_para;
        paz_angleNext = PAZ_ANG;
        paz_b = sin(0.5*(M_PI - PAZ_ANG));
    }
    else if(paz_angleNext < -PAZ_ANG)
    {
        paz_paraNext = paz_r*paz_para;
        paz_angleNext = PAZ_ANG;
        paz_b = sin(0.5*(M_PI - PAZ_ANG));
    }
    if(gl_in[1].gl_Position == gl_in[0].gl_Position)
    {
        paz_paraPrev = paz_para;
        paz_anglePrev = 0;
        paz_a = 1.;
    }
    if(gl_in[3].gl_Position == gl_in[2].gl_Position)
    {
        paz_paraNext = paz_para;
        paz_angleNext = 0;
        paz_b = 1.;
    }
    vec2 paz_perpPrev = vec2(-paz_paraPrev.y, paz_paraPrev.x);
    vec2 paz_perp = vec2(-paz_para.y, paz_para.x);
    vec2 paz_perpNext = vec2(-paz_paraNext.y, paz_paraNext.x);
    vec2 paz_v1 = glLineWidth[1]/paz_a*gl_in[1].gl_Position.w*normalize(paz_perp
        + paz_perpPrev)/paz_size;
    vec2 paz_v2 = glLineWidth[2]/paz_b*gl_in[2].gl_Position.w*normalize(paz_perp
        + paz_perpNext)/paz_size;
    gl_Position = vec4(gl_in[1].gl_Position.xy + paz_v1, gl_in[1].gl_Position.
        zw);
)===";

static const std::string Src2 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy - paz_v1, gl_in[1].gl_Position.
        zw);
)===";

static const std::string Src3 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[2].gl_Position.xy + paz_v2, gl_in[2].gl_Position.
        zw);
)===";

static const std::string Src4 = 1 + R"===(
    EmitVertex();
    gl_Position = vec4(gl_in[2].gl_Position.xy - paz_v2, gl_in[2].gl_Position.
        zw);
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

paz::VertexFunction::Data::~Data()
{
    if(_id)
    {
        glDeleteShader(_id);
    }
    if(_thickLinesId)
    {
        glDeleteShader(_thickLinesId);
    }
}

paz::FragmentFunction::Data::~Data()
{
    if(_id)
    {
        glDeleteShader(_id);
    }
}

paz::VertexFunction::VertexFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::FragmentFunction::FragmentFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::VertexFunction::VertexFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();

    try
    {
        vert2metal(src);
        const std::string fixedSrc = fix_initializers(src);
        const bool usesGlLineWidth = uses_gl_line_width(fixedSrc);
        if(usesGlLineWidth)
        {
            std::string block;
            std::string block0;
            std::string block1;
            const std::string modifiedSrc = prep_for_geom(fixedSrc, block,
                block0, block1);
            _data->_id = compile_shader("#define gl_LineWidth glLineWidth\nout "
                "float glLineWidth;\n" + modifiedSrc, GL_VERTEX_SHADER);
            _data->_thickLinesId = compile_shader(Src0 + block + Src1 + block0 +
                Src2 + block0 + Src3 + block1 + Src4 + block1 + Src5,
                GL_GEOMETRY_SHADER);
        }
        else
        {
            _data->_id = compile_shader(fixedSrc, GL_VERTEX_SHADER);
            _data->_thickLinesId = 0;
        }
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile vertex function: " + std::
            string(e.what()));
    }
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();

    try
    {
        frag2metal(src);
        const std::string fixedSrc = fix_initializers(src);
        _data->_id = compile_shader(fixedSrc, GL_FRAGMENT_SHADER);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile fragment function: " + std::
            string(e.what()));
    }
}

#endif
