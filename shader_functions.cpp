#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "opengl2metal.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include <sstream>
#include <regex>
#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>

static std::unordered_map<unsigned int, unsigned int> get_frag_outputs(const
    std::string& src)
{
    std::unordered_map<unsigned int, unsigned int> outputTypes;
    std::istringstream in(src);
    std::string line;
    while(std::getline(in, line))
    {
        std::size_t l = 0;
        if(std::regex_match(line, std::regex("layout\\b.*")))
        {
            ++l;
            const std::string layout = std::regex_replace(line, std::regex("^.*"
                "location\\s*=\\s*([0-9]+).*$"), "$1");
            const int i = std::stoi(layout);
            if(i < 0)
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Outpu"
                    "t locations must be non-negative.");
            }
            const std::string dec = std::regex_replace(line, std::regex("^.*out"
                "\\s+(.*);$"), "$1");
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            if(outputTypes.count(i))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Outpu"
                    "t location " + std::to_string(i) + " has already been assi"
                    "gned.");
            }
            if(type == "float")
            {
                outputTypes[i] = GL_FLOAT;
            }
            else if(type == "int")
            {
                outputTypes[i] = GL_INT;
            }
            else if(type == "uint")
            {
                outputTypes[i] = GL_UNSIGNED_INT;
            }
            else if(type == "vec2")
            {
                outputTypes[i] = GL_FLOAT_VEC2;
            }
            else if(type == "ivec2")
            {
                outputTypes[i] = GL_FLOAT_VEC2;
            }
            else if(type == "uvec2")
            {
                outputTypes[i] = GL_FLOAT_VEC2;
            }
            else if(type == "vec4")
            {
                outputTypes[i] = GL_FLOAT_VEC4;
            }
            else if(type == "ivec4")
            {
                outputTypes[i] = GL_FLOAT_VEC4;
            }
            else if(type == "uvec4")
            {
                outputTypes[i] = GL_FLOAT_VEC4;
            }
            else
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Outpu"
                    "ts must be scalars, 2-vectors, or 4-vectors.");
            }
        }
    }
    return outputTypes;
}

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

static unsigned int compile_shader(const std::string& src, GLenum type)
{
    // Compile shader.
    unsigned int shader = glCreateShader(type);
    static const std::string headerStr = "#version " + std::to_string(paz::
        GlMajorVersion) + std::to_string(paz::GlMinorVersion) + "0 core\n#defin"
        "e depthSampler2D sampler2D\n";
    std::array<const char*, 2> srcStrs = {headerStr.c_str(), src.c_str()};
    glShaderSource(shader, srcStrs.size(), srcStrs.data(), nullptr);
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
        _data->_id = compile_shader(fixedSrc, GL_VERTEX_SHADER);
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
        _data->_outputTypes = get_frag_outputs(src);
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
