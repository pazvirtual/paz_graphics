#include "opengl2metal.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <regex>

std::string paz::frag2metal(const std::string& src)
{
    std::istringstream in(src);
    std::ostringstream out;

    Mode mode = Mode::None;
    std::stringstream sigBuffer;
    std::stringstream mainBuffer;

    std::unordered_set<std::string> curArgNames;

    std::size_t numOpen = 0;
    std::size_t numClose = 0;

    bool usesGlFragDepth = false;
    bool usesGlPointCoord = false;

    std::unordered_map<std::string, std::pair<std::string, bool>> buffers;
    std::unordered_map<std::string, std::string> textures;
    std::map<std::string, std::pair<std::string, bool>> inputs;
    std::map<unsigned int, std::pair<std::string, std::string>> outputs;
    std::unordered_set<std::string> structs;

    // Include headers.
    out << "#include <metal_stdlib>" << std::endl << "#include <simd/simd.h>" <<
        std::endl << "using namespace metal;" << std::endl;

    // Make it easier to pass around textures and their samplers.
    out << 1 + R"===(
template<typename T>
struct wrap_texture1d
{
    texture1d<T> t;
    sampler s;
};
template<typename T>
struct wrap_texture2d
{
    texture2d<T> t;
    sampler s;
};
template<typename T>
struct wrap_depth2d
{
    depth2d<T> t;
    sampler s;
};
#define sampler1D const wrap_texture1d<float>&
#define sampler2D const wrap_texture2d<float>&
#define isampler1D const wrap_texture1d<int>&
#define isampler2D const wrap_texture2d<int>&
#define usampler1D const wrap_texture1d<uint>&
#define usampler2D const wrap_texture2d<uint>&
#define depthSampler2D const wrap_depth2d<float>&
)===";

    // Define our Y-reversed sample functions.
    out << 1 + R"===(
template<typename T>
auto texture(thread const wrap_texture1d<T>& tex, thread float u)
{
    return tex.t.sample(tex.s, u);
}
template<typename T>
auto textureLod(thread const wrap_texture1d<T>& tex, thread float u, thread
    float lod)
{
    return tex.t.sample(tex.s, u, level(lod));
}
template<typename T>
auto texture(thread const wrap_texture2d<T>& tex, thread const float2& uv)
{
    return tex.t.sample(tex.s, float2(uv.x, 1. - uv.y));
}
template<typename T>
auto textureLod(thread const wrap_texture2d<T>& tex, thread const float2& uv,
    thread float lod)
{
    return tex.t.sample(tex.s, float2(uv.x, 1. - uv.y), level(lod));
}
float4 texture(thread const wrap_depth2d<float>& tex, thread const float2& uv)
{
    return float4(tex.t.sample(tex.s, float2(uv.x, 1. - uv.y)), 0, 0, 1);
}
float4 textureLod(thread const wrap_depth2d<float>& tex, thread const float2&
    uv, thread float lod)
{
    return float4(tex.t.sample(tex.s, float2(uv.x, 1. - uv.y), level(lod)), 0,
        0, 1);
}
)===";

    // Define `textureQueryLod()`. Note that this is not the right formula for
    // lines and that Metal does not support mipmaps for 1D textures.
    out << 1 + R"===(
template<typename T>
float2 textureQueryLod(thread const wrap_texture1d<T>& /* tex */, thread const
    float2& /* uv */)
{
    return float2(0);
}
template<typename T>
float2 textureQueryLod(thread const wrap_texture2d<T>& tex, thread const float2&
    uv)
{
    const float2 size(tex.t.get_width(), tex.t.get_height());
    const float2 duvdx = dfdx(uv);
    const float2 duvdy = dfdy(uv);
    const float rho = max(length(size*duvdx), length(size*duvdy));
    const float lambdaPrime = log2(rho);
    return float2(clamp(lambdaPrime, 0., float(tex.t.get_num_mip_levels())),
        lambdaPrime);
}
template<typename T>
float2 textureQueryLod(thread const wrap_depth2d<T>& tex, thread const float2&
    uv)
{
    const float2 size(tex.t.get_width(), tex.t.get_height());
    const float2 duvdx = dfdx(uv);
    const float2 duvdy = dfdy(uv);
    const float rho = max(length(size*duvdx), length(size*duvdy));
    const float lambdaPrime = log2(rho);
    return float2(clamp(lambdaPrime, 0., float(tex.t.get_num_mip_levels())),
        lambdaPrime);
}
)===";

    // Define `textureSize()`.
    out << 1 + R"===(
template<typename T>
int textureSize(thread const wrap_texture1d<T>& tex, thread int lod)
{
    return tex.t.get_width(lod);
}
template<typename T>
int2 textureSize(thread const wrap_texture2d<T>& tex, thread int lod)
{
    return int2(tex.t.get_width(lod), tex.t.get_height(lod));
}
template<typename T>
int2 textureSize(thread const wrap_depth2d<T>& tex, thread int lod)
{
    return int2(tex.t.get_width(lod), tex.t.get_height(lod));
}
)===";

    // Define reinterpretation functions.
    out << 1 + R"===(
auto floatBitsToInt(float v)
{
    return as_type<int>(v);
}
auto floatBitsToInt(thread const float2& v)
{
    return as_type<int2>(v);
}
auto floatBitsToInt(thread const float3& v)
{
    return as_type<int3>(v);
}
auto floatBitsToInt(thread const float4& v)
{
    return as_type<int4>(v);
}
auto floatBitsToUint(float v)
{
    return as_type<uint>(v);
}
auto floatBitsToUint(thread const float2& v)
{
    return as_type<uint2>(v);
}
auto floatBitsToUint(thread const float3& v)
{
    return as_type<uint3>(v);
}
auto floatBitsToUint(thread const float4& v)
{
    return as_type<uint4>(v);
}
auto intBitsToFloat(int v)
{
    return as_type<float>(v);
}
auto intBitsToFloat(thread const int2& v)
{
    return as_type<float2>(v);
}
auto intBitsToFloat(thread const int3& v)
{
    return as_type<float3>(v);
}
auto intBitsToFloat(thread const int4& v)
{
    return as_type<float4>(v);
}
auto uintBitsToFloat(uint v)
{
    return as_type<float>(v);
}
auto uintBitsToFloat(thread const uint2& v)
{
    return as_type<float2>(v);
}
auto uintBitsToFloat(thread const uint3& v)
{
    return as_type<float3>(v);
}
auto uintBitsToFloat(thread const uint4& v)
{
    return as_type<float4>(v);
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
        if(std::regex_match(line, std::regex(".*\\b(depthS|[iu]?s)ampler[34]D\\"
            "b.*")))
        {
            throw std::runtime_error("Higher-dimensional textures are not suppo"
                "rted.");
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

        // Check for struct type declarations.
        if(std::regex_match(line, std::regex("\\s*struct\\s+[a-zA-Z_][a-zA-Z_0-"
            "9]*\\b.*")))
        {
            structs.insert(std::regex_replace(line, std::regex("^\\s*struct\\s+"
                "([a-zA-Z_][a-zA-Z_0-9]*)\\b"), "$1"));
        }

        // Do this check here and save the result.
        const bool isFun = mode == Mode::None && std::regex_match(line, std::
            regex("\\s*[a-zA-Z_][a-zA-Z_0-9]*\\s+[a-zA-Z_][a-zA-Z_0-9]*\\(.*"));

        // Check for inputs and outputs out of scope.
        if(mode != Mode::Main)
        {
            if(std::regex_match(line, std::regex(".*\\bgl_Frag(Depth|Coord)\\b."
                "*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r outputs cannot be accessed outside of main function.");
            }
            if(!isFun && mode != Mode::Sig)
            {
                for(const auto& n : outputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.second.
                        first + "\\b.*")) && !curArgNames.count(n.second.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader outputs cannot be accessed outside of mai"
                            "n function.");
                    }
                }
                for(const auto& n : inputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.first +
                        "\\b.*")) && !curArgNames.count(n.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader inputs cannot be accessed outside of main"
                            " function.");
                    }
                }
                for(const auto& n : textures)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.first +
                        "\\b.*")) && !curArgNames.count(n.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) +
                            ": Shader uniforms cannot be accessed outside of ma"
                            "in function.");
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
        line = std::regex_replace(line, std::regex("\\bdiscard\\b"),
            "discard_fragment()");

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
                std::string sig;
                try
                {
                    sig = process_sig(sigBuffer.str(), curArgNames);
                }
                catch(const std::exception& e)
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": F"
                        "ailed to process private function signature: " + e.
                        what());
                }
                out << sig << std::endl;
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
                line = std::regex_replace(line, std::regex("\\b" + n.first +
                    "\\b"), "in." + n.first);
            }
            for(const auto& n : outputs)
            {
                line = std::regex_replace(line, std::regex("\\b" + n.second.
                    first + "\\b"), "out." + n.second.first);
            }
            if(!usesGlFragDepth && std::regex_match(line, std::regex(".*\\bgl_F"
                "ragDepth\\b.*")))
            {
                usesGlFragDepth = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_FragDepth\\b"),
                "out.glFragDepth");
            line = std::regex_replace(line, std::regex("\\bgl_FragCoord\\b"),
                "in.glFragCoord");
            if(!usesGlPointCoord && std::regex_match(line, std::regex(".*\\bgl_"
                "PointCoord\\b.*")))
            {
                usesGlPointCoord = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_PointCoord\\b"),
                "in.glPointCoord");
            mainBuffer << line << std::endl;
            continue;
        }

        // Process uniforms, inputs, and outputs.
        if(std::regex_match(line, std::regex("uniform\\s+.*")))
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            std::string type = dec.substr(0, pos);
            std::string name = dec.substr(pos + 1);
            if(type.back() == 'D')
            {
                if(type == "sampler1D")
                {
                    type = "texture1d<float>";
                }
                else if(type == "sampler2D")
                {
                    type = "texture2d<float>";
                }
                else if(type == "isampler1D")
                {
                    type = "texture1d<int>";
                }
                else if(type == "isampler2D")
                {
                    type = "texture2d<int>";
                }
                else if(type == "usampler1D")
                {
                    type = "texture1d<uint>";
                }
                else if(type == "usampler2D")
                {
                    type = "texture2d<uint>";
                }
                else if(type == "depthSampler2D")
                {
                    type = "depth2d<float>";
                }
                else
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": U"
                        "nsupported texture type \"" + type + "\".");
                }
                textures[name] = type;
            }
            else
            {
                const bool isPointer = (name.back() == ']');
                if(isPointer)
                {
                    name = name.substr(0, name.find('['));
                }
                buffers[name] = {type, isPointer};
            }
            continue;
        }
        if(std::regex_match(line, std::regex("in\\s+.*")))
        {
            const std::string dec = line.substr(3, line.size() - 4);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs[name] = {type, false};
            continue;
        }
        if(std::regex_match(line, std::regex("flat in\\s+.*")))
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs[name] = {type, true};
            continue;
        }
        if(std::regex_match(line, std::regex("layout\\b.*")))
        {
            const std::string layout = std::regex_replace(line, std::regex("^.*"
                "location\\s*=\\s*([0-9]+).*$"), "$1");
            const int i = std::stoi(layout);
            const std::string dec = std::regex_replace(line, std::regex("^.*out"
                "\\s+(.*);$"), "$1");
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            outputs[i] = {name, type};
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
                std::string sig;
                try
                {
                    sig = process_sig(line, curArgNames);
                }
                catch(const std::exception& e)
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": F"
                        "ailed to process private function signature: " + e.
                        what());
                }
                out << sig << std::endl;
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

    // Append main function.
    out << "struct InputData" << std::endl << "{" << std::endl << "    float4 g"
        "lFragCoord [[position]];" << std::endl;
    if(usesGlPointCoord)
    {
        out << "    float2 glPointCoord [[point_coord]];" << std::endl;
    }
    for(const auto& n : inputs)
    {
        out << "    " << n.second.first << " " << n.first << (n.second.second ?
            " [[flat]]" : "") << ";" << std::endl;
    }
    out << "};" << std::endl;
    out << "struct OutputData" << std::endl << "{" << std::endl;
    for(const auto& n : outputs)
    {
        out << "    " << n.second.second << " " << n.second.first << " [[color("
            << n.first << ")]];" << std::endl;
    }
    if(usesGlFragDepth)
    {
        out << "    float glFragDepth [[depth(any)]];" << std::endl;
    }
    out << "};" << std::endl;
    out << "fragment OutputData fragMain(InputData in [[stage_in]]";
    int b = 0;
    for(const auto& n : buffers)
    {
        out << ", constant " << n.second.first << (n.second.second ? "* " :
            "& ") << n.first << " [[buffer(" << b << ")]]";
        ++b;
    }
    int t = 0;
    for(const auto& n : textures)
    {
        out << ", " << n.second << " " << n.first << "Texture [[texture(" << t
            << ")]]" << ", sampler " << n.first << "Sampler [[sampler(" << t <<
            ")]]";
        ++t;
    }
    out << ")" << std::endl << "{" << std::endl;
    for(const auto& n : textures)
    {
        out << "    const wrap_" << n.second << " " << n.first << " = {" << n.
            first << "Texture, " << n.first << "Sampler};" << std::endl;
    }
    out << "    OutputData out;" << std::endl << mainBuffer.str() << "    retur"
        "n out;" << std::endl << "}" << std::endl;

    return out.str();
}
