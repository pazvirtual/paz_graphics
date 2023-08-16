#include "shading_lang.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <regex>

static const std::vector<std::string> unsupportedBuiltins = {"gl_PerVertex",
    "gl_ClipDistance", "gl_Pointsize"};

std::string paz::vert2hlsl(const std::string& src)
{
    std::istringstream in(src);
    std::ostringstream out;

    Mode mode = Mode::None;
    std::stringstream sigBuffer;
    std::stringstream mainBuffer;

    std::unordered_set<std::string> curArgNames;

    std::size_t numOpen = 0;
    std::size_t numClose = 0;

    bool usesGlPosition = false;
    bool usesGlVertexId = false;
    bool usesGlInstanceId = false;

    std::unordered_map<std::string, std::pair<std::string, int>> buffers;
    std::map<int, std::pair<std::string, std::string>> inputs;
    std::vector<std::pair<std::string, std::pair<std::string, bool>>> outputs;
    std::unordered_set<std::string> structs;

    // Define reinterpretation functions.
    out << 1 + R"===(
int floatBitsToInt(in float v)
{
    return asint(v);
}
int2 floatBitsToInt(in float2 v)
{
    return asint(v);
}
int3 floatBitsToInt(in float3 v)
{
    return asint(v);
}
int4 floatBitsToInt(in float4 v)
{
    return asint(v);
}
uint floatBitsToUint(in float v)
{
    return asuint(v);
}
uint2 floatBitsToUint(in float2 v)
{
    return asuint(v);
}
uint3 floatBitsToUint(in float3 v)
{
    return asuint(v);
}
uint4 floatBitsToUint(in float4 v)
{
    return asuint(v);
}
float intBitsToFloat(in int v)
{
    return asfloat(v);
}
float2 intBitsToFloat(in int2 v)
{
    return asfloat(v);
}
float3 intBitsToFloat(in int3 v)
{
    return asfloat(v);
}
float4 intBitsToFloat(in int4 v)
{
    return asfloat(v);
}
float uintBitsToFloat(in uint v)
{
    return asfloat(v);
}
float2 uintBitsToFloat(in uint2 v)
{
    return asfloat(v);
}
float3 uintBitsToFloat(in uint3 v)
{
    return asfloat(v);
}
float4 uintBitsToFloat(in uint4 v)
{
    return asfloat(v);
}
)===";

    std::string line;
    std::size_t l = 0;
    while(std::getline(in, line))
    {
        ++l;

        // Clean up lines.
        line = std::regex_replace(line, std::regex("//.*$"), "");
        line = std::regex_replace(line, std::regex("\\s+$"), "");
        if(line.empty())
        {
            continue;
        }

        // Check for macro definitions and other unsupported features.
        if(line.substr(0, 8) == "#version" || line.substr(0, 10) ==
            "#extension")
        {
            throw std::runtime_error("Line " + std::to_string(l) + ": Version a"
                "nd extension directives are not supported.");
        }
        if(line.substr(0, 7) == "#define")
        {
            throw std::runtime_error("Line " + std::to_string(l) + ": User-defi"
                "ned macros are not supported.");
        }
        if(std::regex_match(line, std::regex(".*\\b(depthS|[iu]?s)ampler[1-4]D"
            "\\b.*")))
        {
            throw std::runtime_error("Texture sampling in vertex shaders is not"
                " supported.");
        }
        if(std::regex_match(line, std::regex(".*\\b(float|u?int|[iu]?vec[2-4])"
            "\\s*\\[\\s*[a-zA-Z_0-9]*\\s*\\]\\s*\\(.*")))
        {
            throw std::runtime_error("Only braced initializer lists are support"
                "ed for array definition.");
        }
        for(const auto& n : structs)
        {
            if(std::regex_match(line, std::regex(".*\\b" + n + "\\s*\\(.*")))
            {
                throw std::runtime_error("Only braced initializer lists are sup"
                    "ported for struct definition.");
            }
        }
        if(std::regex_match(line, std::regex(".*\\b(float|u?int|[iu]?vec[2-4])"
            "\\s*\\[.*")))
        {
            throw std::runtime_error("Array dimensions must follow variable nam"
                "e.");
        }
        if(std::regex_match(line, std::regex(".*\\blength\\(\\s*\\).*")))
        {
            throw std::runtime_error("Array length method is not supported.");
        }
        if(std::regex_match(line, std::regex(".*\\binverse\\b.*")))
        {
            throw std::runtime_error("Matrix inverse is not supported.");
        }

        // Keep macro conditionals.
        if(std::regex_match(line, std::regex("\\s*#((end)?if|else|ifn?def).*")))
        {
            out << line << std::endl;
            continue;
        }

        // Check for unsupported built-in outputs.
        for(const auto& n : unsupportedBuiltins)
        {
            if(std::regex_match(line, std::regex(".*\\b" + n + "\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r output \"" + n + "\" is not currently supported.");
            }
        }

        // Do this check here and save the result.
        const bool isFun = mode == Mode::None && std::regex_match(line, std::
            regex("\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s+[a-zA-Z_][a-zA-Z_0-9]*\\(.*"));

        // Check for struct type declarations.
        if(std::regex_match(line, std::regex("\\s*struct\\s+[a-zA-Z_][a-zA-Z_0-"
            "9]*\\b.*")))
        {
            structs.insert(std::regex_replace(line, std::regex("^\\s*struct\\s+"
                "([a-zA-Z_][a-zA-Z_0-9]*)\\b"), "$1"));
        }

        // Check for inputs and outputs out of scope.
        if(mode != Mode::Main)
        {
            if(std::regex_match(line, std::regex(".*\\bgl_Position\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r outputs cannot be accessed outside of main function.");
            }
            if(std::regex_match(line, std::regex(".*\\bgl_(Vertex|Instance)ID\\"
                "b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r inputs cannot be accessed outside of main function.");
            }
            if(!isFun && mode != Mode::Sig)
            {
                for(const auto& n : outputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.first +
                        "\\b.*")) && !curArgNames.count(n.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader outputs cannot be accessed outside of mai"
                            "n function.");
                    }
                }
                for(const auto& n : inputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + std::get<0>(
                        n.second) + "\\b.*")) && !curArgNames.count(std::get<0>(
                        n.second)))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader inputs cannot be accessed outside of main"
                            " function.");
                    }
                }
                for(const auto& n : buffers)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.first +
                        "\\b.*")) && !curArgNames.count(n.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader uniforms cannot be accessed outside of ma"
                            "in function.");
                    }
                }
            }
        }

        // End private function.
        if(mode == Mode::Fun && line == "}")
        {
            out << line << std::endl;
            mode = Mode::None;
            curArgNames.clear();
            continue;
        }

        // Detect beginning of main function.
        if(line == "void main()")
        {
            mode = Mode::Main;
            continue;
        }

        // Apply global definitions.
        line = std::regex_replace(line, std::regex("\\bmat([2-4])x([2-4])"),
            "float$1x$2");
        line = std::regex_replace(line, std::regex("\\bmat([2-4])"),
            "float$1x$1");
        line = std::regex_replace(line, std::regex("\\bvec([2-4])"), "float$1");
        line = std::regex_replace(line, std::regex("\\bivec([2-4])"), "int$1");
        line = std::regex_replace(line, std::regex("\\buvec([2-4])"), "uint$1");

        // Process global variables.
        if(mode == Mode::None && std::regex_match(line, std::regex("\\s*const\\"
            "s.*=.*")))
        {
            line = std::regex_replace(line, std::regex("\\s*const\\b"),
                "constant");
            out << line << std::endl;
            continue;
        }

        // Handle struct definitions.
        if(mode == Mode::None && std::regex_match(line, std::regex("\\s*struct"
            "\\s[a-zA-Z_][a-zA-Z_0-9]*")))
        {
            out << line << std::endl;
            mode = Mode::Struct;
            continue;
        }
        if(mode == Mode::Struct)
        {
            out << line << std::endl;
            if(line == "};")
            {
                mode = Mode::None;
            }
            continue;
        }

        // Continue processing private function signature (see below).
        if(mode == Mode::Sig)
        {
            line = std::regex_replace(line, std::regex("^\\s+"), "");
            sigBuffer << " " << line;

            // Check if signature is complete.
            numOpen += std::count(line.begin(), line.end(), '(');
            numClose += std::count(line.begin(), line.end(), ')');
            if(numOpen == numClose)
            {
                mode = Mode::Fun;
                try
                {
                    process_sig(sigBuffer.str(), curArgNames);
                }
                catch(const std::exception& e)
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": F"
                        "ailed to process private function signature: " + e.
                        what());
                }
                out << line << std::endl;
                sigBuffer.clear();
                numOpen = 0;
                numClose = 0;
            }
            continue;
        }

        // Preserve body of private function.
        if(mode == Mode::Fun)
        {
            out << line << std::endl;
            continue;
        }

        // Process main function lines.
        if(mode == Mode::Main)
        {
            if(line == "{" || line == "}")
            {
                continue;
            }
            for(const auto& n : inputs)
            {
                line = std::regex_replace(line, std::regex("\\b" + n.second.
                    first + "\\b"), "input." + n.second.first);
            }
            for(const auto& n : outputs)
            {
                line = std::regex_replace(line, std::regex("\\b" + n.first +
                    "\\b"), "output." + n.first);
            }
            if(!usesGlPosition && std::regex_match(line, std::regex(".*\\bgl_Po"
                "sition\\b.*")))
            {
                usesGlPosition = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_Position\\b"),
                "output.glPosition");
            if(!usesGlVertexId && std::regex_match(line, std::regex(".*\\bgl_Ve"
                "rtexID\\b.*")))
            {
                usesGlVertexId = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_VertexID\\b"),
                "glVertexId");
            if(!usesGlInstanceId && std::regex_match(line, std::regex(".*\\bgl_"
                "InstanceID\\b.*")))
            {
                usesGlInstanceId = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_InstanceID\\b"),
                "glInstanceId");
            mainBuffer << line << std::endl;
            continue;
        }

        // Process uniforms, inputs, and outputs.
        if(std::regex_match(line, std::regex("uniform\\s.*")))
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            std::string name = dec.substr(pos + 1);
            int size = -1;
            if(name.back() == ']')
            {
                const auto pos = name.find('[');
                size = std::stoi(name.substr(pos + 1, name.size() - pos - 2));
                name = name.substr(0, pos);
            }
            buffers[name] = {type, size};
            continue;
        }
        if(std::regex_match(line, std::regex("layout\\b.*")))
        {
            const std::string layout = std::regex_replace(line, std::regex("^.*"
                "location\\s*=\\s*([0-9]+).*$"), "$1");
            const int i = std::stoi(layout);
            const bool isInstance = std::regex_match(line, std::regex(".*\\[\\["
                "\\s*instance\\s*\\]\\].*"));
            usesGlInstanceId |= isInstance;
            const std::string dec = std::regex_replace(line, std::regex("^.*in"
                "\\s+(.*)\\s" + std::string(isInstance ? "+\\[\\[\\s*instance\\"
                "s*\\]\\]\\s*" : "*") + ";$"), "$1");
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs[i] = {name, type};
            continue;
        }
        if(std::regex_match(line, std::regex("out\\s.*")))
        {
            const std::string dec = line.substr(4, line.size() - 5);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            outputs.push_back({name, {type, false}});
            continue;
        }
        if(std::regex_match(line, std::regex("flat out\\s.*")))
        {
            const std::string dec = line.substr(9, line.size() - 10);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            outputs.push_back({name, {type, true}});
            continue;
        }

        // Process private functions.
        if(isFun)
        {
            // Check if the signature is complete.
            numOpen = std::count(line.begin(), line.end(), '(');
            numClose = std::count(line.begin(), line.end(), ')');
            if(numOpen == numClose)
            {
                mode = Mode::Fun;
                try
                {
                    process_sig(line, curArgNames);
                }
                catch(const std::exception& e)
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": F"
                        "ailed to process private function signature: " + e.
                        what());
                }
                out << line << std::endl;
            }
            else
            {
                mode = Mode::Sig;
                sigBuffer << line;
            }
            continue;
        }

        // Throw if none of the above applied to this line.
        throw std::runtime_error("Line " + std::to_string(l) + ": Unable to pro"
            "cess line: \"" + line + "\".");
    }

    if(!usesGlPosition)
    {
        throw std::runtime_error("Vertex position not set in shader.");
    }

    // Append main function.
    for(const auto& n : buffers)
    {
        out << "uniform " << n.second.first << " " << n.first << (n.second.
            second < 0 ? "" : "[" + std::to_string(n.second.second) + "]") <<
            ";" << std::endl;
    }
    out << "struct InputData" << std::endl << "{" << std::endl;
    for(const auto& n : inputs)
    {
        out << "    " << n.second.second << " " << n.second.first << " : ATTR"
            << n.first << ";" << std::endl;
    }
    if(usesGlVertexId)
    {
        out << "    uint glVertexId : SV_VertexID;" << std::endl;
    }
    if(usesGlInstanceId)
    {
        out << "    uint glInstanceId : SV_InstanceID;" << std::endl;;
    }
    out << "};" << std::endl;
    out << "struct OutputData" << std::endl << "{" << std::endl << "    float4 "
        "glPosition : SV_Position;" << std::endl;
    for(std::size_t i = 0; i < outputs.size(); ++i)
    {
        out << "    " << (outputs[i].second.second ? "nointerpolation " : "") <<
            outputs[i].second.first << " " << outputs[i].first << " : TEXCOORD"
            << i << ";" << std::endl;
    }
    out << "};" << std::endl;
    out << "OutputData main(InputData input)" << std::endl << "{" << std::endl
        << "    OutputData output;" << std::endl << mainBuffer.str() << "    ou"
        "tput.glPosition = mul(output.glPosition, float4x4(1, 0, 0, 0, 0, 1, 0,"
        " 0," << std::endl << "        0, 0, 0.5, 0.5, 0, 0, 0, 1));" << std::
        endl << "    return output;" << std::endl << "}" << std::endl;

    return out.str();
}
