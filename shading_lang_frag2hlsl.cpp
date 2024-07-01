#include "shading_lang.hpp"
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <regex>

static const std::vector<std::string> unsupportedBuiltins = {"gl_FragCoord"};

std::string paz::frag2hlsl(const std::string& src, std::vector<std::tuple<std::
    string, DataType, int, int>>& uniforms)
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

    std::vector<std::pair<std::string, std::pair<std::string, int>>> buffers;
    std::unordered_map<std::string, std::pair<std::string, std::string>>
        textures;
    std::vector<std::pair<std::string, std::pair<std::string, bool>>> inputs;
    std::map<int, std::pair<std::string, std::string>> outputs;
    std::unordered_set<std::string> structs;

    // Make it easier to pass around textures and their samplers.
    out << 1 + R"===(
struct wrap_sampler1D
{
    Texture1D t;
    SamplerState s;
};
struct wrap_isampler1D
{
    Texture1D<int4> t;
    SamplerState s;
};
struct wrap_usampler1D
{
    Texture1D<uint4> t;
    SamplerState s;
};
struct wrap_sampler2D
{
    Texture2D t;
    SamplerState s;
};
struct wrap_isampler2D
{
    Texture2D<int4> t;
    SamplerState s;
};
struct wrap_usampler2D
{
    Texture2D<uint4> t;
    SamplerState s;
};
#define sampler1D wrap_sampler1D
#define sampler2D wrap_sampler2D
#define isampler1D wrap_isampler1D
#define isampler2D wrap_isampler2D
#define usampler1D wrap_usampler1D
#define usampler2D wrap_usampler2D
)===";

    // Define our Y-reversed sample functions.
    out << 1 + R"===(
float4 sample_texture(in wrap_sampler1D tex, in float u)
{
    return tex.t.Sample(tex.s, u);
}
int4 sample_texture(in wrap_isampler1D tex, in float u)
{
    return tex.t.Sample(tex.s, u);
}
uint4 sample_texture(in wrap_usampler1D tex, in float u)
{
    return tex.t.Sample(tex.s, u);
}
float4 textureLod(in wrap_sampler1D tex, in float u, in float lod)
{
    return tex.t.SampleLevel(tex.s, u, lod);
}
int4 textureLod(in wrap_isampler1D tex, in float u, in float lod)
{
    return tex.t.SampleLevel(tex.s, u, lod);
}
uint4 textureLod(in wrap_usampler1D tex, in float u, in float lod)
{
    return tex.t.SampleLevel(tex.s, u, lod);
}
float4 sample_texture(in wrap_sampler2D tex, in float2 uv)
{
    return tex.t.Sample(tex.s, float2(uv.x, 1. - uv.y));
}
int4 sample_texture(in wrap_isampler2D tex, in float2 uv)
{
    return tex.t.Sample(tex.s, float2(uv.x, 1. - uv.y));
}
uint4 sample_texture(in wrap_usampler2D tex, in float2 uv)
{
    return tex.t.Sample(tex.s, float2(uv.x, 1. - uv.y));
}
float4 textureLod(in wrap_sampler2D tex, in float2 uv, in float lod)
{
    return tex.t.SampleLevel(tex.s, float2(uv.x, 1. - uv.y), lod);
}
int4 textureLod(in wrap_isampler2D tex, in float2 uv, in float lod)
{
    return tex.t.SampleLevel(tex.s, float2(uv.x, 1. - uv.y), lod);
}
uint4 textureLod(in wrap_usampler2D tex, in float2 uv, in float lod)
{
    return tex.t.SampleLevel(tex.s, float2(uv.x, 1. - uv.y), lod);
}
#define texture sample_texture
)===";

    // Define `textureQueryLod()`. Should check that these are correct.
//    out << 1 + R"===(
//float2 textureQueryLod(in wrap_sampler1D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//int2 textureQueryLod(in wrap_isampler1D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//uint2 textureQueryLod(in wrap_usampler1D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//float2 textureQueryLod(in wrap_sampler2D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//int2 textureQueryLod(in wrap_isampler2D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//uint2 textureQueryLod(in wrap_usampler2D tex, in float u)
//{
//    return float2(tex.t.CalculateLevelOfDetail(tex.s, u), tex.t.
//        CalculateLevelOfDetailUnclamped(tex.s, u));
//}
//)===";

    // Define `textureSize()`. Note that LOD parameter is currently ignored.
    out << 1 + R"===(
int textureSize(wrap_sampler1D tex, int lod)
{
    uint w;
    tex.t.GetDimensions(w);
    return w;
}
int textureSize(wrap_isampler1D tex, int lod)
{
    uint w;
    tex.t.GetDimensions(w);
    return w;
}
int textureSize(wrap_usampler1D tex, int lod)
{
    uint w;
    tex.t.GetDimensions(w);
    return w;
}
int2 textureSize(wrap_sampler2D tex, int lod)
{
    uint w, h;
    tex.t.GetDimensions(w, h);
    return int2(w, h);
}
int2 textureSize(wrap_isampler2D tex, int lod)
{
    uint w, h;
    tex.t.GetDimensions(w, h);
    return int2(w, h);
}
int2 textureSize(wrap_usampler2D tex, int lod)
{
    uint w, h;
    tex.t.GetDimensions(w, h);
    return int2(w, h);
}
)===";

    // Define reinterpretation functions.
    out << 1 + R"===(
//int floatBitsToInt(in float v)
//{
//    return asint(v);
//}
//int2 floatBitsToInt(in float2 v)
//{
//    return asint(v);
//}
//int3 floatBitsToInt(in float3 v)
//{
//    return asint(v);
//}
//int4 floatBitsToInt(in float4 v)
//{
//    return asint(v);
//}
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

    // Fix HLSL matrix issues. (WIP)
    for(int i = 2; i <= 4; ++i)
    {
        for(int j = 2; j <= 4; ++j)
        {
            out << "float" << i << "x" << j << " paz_make_float" << i << "x" <<
                j << "(";
            for(int x = 0; x < i; ++x)
            {
                for(int y = 0; y < j; ++y)
                {
                    out << "in float a" << x << y;
                    if(x + 1 < i || y + 1 < j)
                    {
                        out << ", ";
                    }
                }
            }
            out << ")\n{\n    return float" << i << "x" << j << "(";
            for(int y = 0; y < j; ++y)
            {
                for(int x = 0; x < i; ++x)
                {
                    out << "a" << x << y;
                    if(x + 1 < i || y + 1 < j)
                    {
                        out << ", ";
                    }
                }
            }
            out << ");\n}\n";
        }
    }

    // Define GLSL-like modulo.
    out << 1 + R"===(
float mod(in float x, in float y)
{
    return x - y*floor(x/y);
}
float2 mod(in float2 x, in float y)
{
    return x - y*floor(x/y);
}
float3 mod(in float3 x, in float y)
{
    return x - y*floor(x/y);
}
float4 mod(in float4 x, in float y)
{
    return x - y*floor(x/y);
}
float2 mod(in float2 x, in float2 y)
{
    return x - y*floor(x/y);
}
float3 mod(in float3 x, in float3 y)
{
    return x - y*floor(x/y);
}
float4 mod(in float4 x, in float4 y)
{
    return x - y*floor(x/y);
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
        for(const auto& n : unsupportedBuiltins)
        {
            if(std::regex_match(line, std::regex(".*\\b" + n + "\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r input \"" + n + "\" is not currently supported.");
            }
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
        line = std::regex_replace(line, std::regex("\\bmat([2-4])\\s*\\("),
            "paz_make_float$1x$1(");
        line = std::regex_replace(line, std::regex("\\bmat([2-4])x([2-4])\\s*\\"
            "("), "paz_make_float$1x$2(");
        line = std::regex_replace(line, std::regex("\\bmat([2-4])x([2-4])"),
            "float$1x$2");
        line = std::regex_replace(line, std::regex("\\bmat([2-4])"),
            "float$1x$1");
        line = std::regex_replace(line, std::regex("\\bvec([2-4])"), "float$1");
        line = std::regex_replace(line, std::regex("\\bivec([2-4])"), "int$1");
        line = std::regex_replace(line, std::regex("\\buvec([2-4])"), "uint$1");
        line = std::regex_replace(line, std::regex("\\bmix\\b"), "lerp");
        line = std::regex_replace(line, std::regex("\\bfract\\b"), "frac");

        // Process global variables.
        if(mode == Mode::None && std::regex_match(line, std::regex("\\s*const\\"
            "s.*=.*")))
        {
            line = std::regex_replace(line, std::regex("\\s*const\\b"),
                "static const");
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
            out << "    " << line << std::endl;

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
                    "\\b"), "input." + n.first);
            }
            for(const auto& n : outputs)
            {
                line = std::regex_replace(line, std::regex("\\b" + n.second.
                    first + "\\b"), "output." + n.second.first);
            }
            if(!usesGlFragDepth && std::regex_match(line, std::regex(".*\\bgl_F"
                "ragDepth\\b.*")))
            {
                usesGlFragDepth = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_FragDepth\\b"),
                "output.glFragDepth");
            line = std::regex_replace(line, std::regex("\\bgl_FragCoord\\b"),
                "input.glFragCoord");
            if(!usesGlPointCoord && std::regex_match(line, std::regex(".*\\bgl_"
                "PointCoord\\b.*")))
            {
                usesGlPointCoord = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_PointCoord\\b"),
                "input.glPointCoord");
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
            std::string texType;
            if(type.back() == 'D')
            {
                if(type == "depthSampler2D")
                {
                    type = "sampler2D";
                }
                if(type == "sampler1D")
                {
                    texType = "Texture1D";
                }
                else if(type == "sampler2D")
                {
                    texType = "Texture2D";
                }
                else if(type == "isampler1D")
                {
                    texType = "Texture1D<int>";
                }
                else if(type == "isampler2D")
                {
                    texType = "Texture2D<int>";
                }
                else if(type == "usampler1D")
                {
                    texType = "Texture1D<uint>";
                }
                else if(type == "usampler2D")
                {
                    texType = "Texture2D<uint>";
                }
                else
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": U"
                        "nsupported texture type \"" + type + "\".");
                }
                textures[name] = {texType, type};
            }
            else
            {
                int size = -1;
                if(name.back() == ']')
                {
                    const auto pos = name.find('[');
                    size = std::stoi(name.substr(pos + 1, name.size() - pos -
                        2));
                    name = name.substr(0, pos);
                }
                buffers.push_back({name, {type, size}});
            }
            continue;
        }
        if(std::regex_match(line, std::regex("in\\s+.*")))
        {
            const std::string dec = line.substr(3, line.size() - 4);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs.push_back({name, {type, false}});
            continue;
        }
        if(std::regex_match(line, std::regex("flat in\\s+.*")))
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs.push_back({name, {type, true}});
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
            out << line << std::endl;

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
    for(const auto& n : buffers)
    {
        out << "uniform " << n.second.first << " " << n.first << (n.second.
            second < 0 ? "" : "[" + std::to_string(n.second.second) + "]") <<
            ";" << std::endl;
        auto size = n.second.second < 0 ? 1 : n.second.second;
        DataType type;
        int len;
        if(n.second.first.substr(0, 3) == "int")
        {
            type = DataType::SInt;
            len = 3;
        }
        else if(n.second.first.substr(0, 4) == "uint")
        {
            type = DataType::UInt;
            len = 4;
        }
        else if(n.second.first.substr(0, 5) == "float")
        {
            type = DataType::Float;
            len = 5;
        }
        else
        {
            throw std::runtime_error("Unsupported uniform type \"" + n.second.
                first + "\".");
        }
        const auto suffix = n.second.first.substr(len);
        int rows, cols;
        if(suffix.empty())
        {
            rows = 1;
            cols = 1;
        }
        else if(suffix.size() == 1 && suffix[0] >= '1' && suffix[0] <= '4')
        {
            rows = suffix[0] - '0';
            cols = 1;
        }
        else if(suffix.size() == 3 && suffix[0] >= '1' && suffix[0] <= '4' &&
            suffix[1] == 'x' && suffix[2] >= '1' && suffix[2] <= '4')
        {
            rows = suffix[0] - '0';
            cols = suffix[2] - '0';
        }
        else
        {
            throw std::runtime_error("Unsupported uniform type \"" + n.second.
                first + "\".");
        }
        uniforms.push_back({n.first, type, rows*cols, size});
    }
    for(const auto& n : textures)
    {
        out << "uniform " << n.second.first << " " << n.first << "Texture;" <<
            std::endl << "uniform SamplerState " << n.first << "Sampler;" <<
            std::endl;
    }
    out << "struct InputData" << std::endl << "{" << std::endl << "    float4 g"
        "lFragCoord : SV_Position;" << std::endl;
    if(usesGlPointCoord)
    {
        out << "    float2 glPointCoord : SV_Position;" << std::endl;
    }
    for(std::size_t i = 0; i < inputs.size(); ++i)
    {
        out << "    " << (inputs[i].second.second ? "nointerpolation " : "") <<
            inputs[i].second.first << " " << inputs[i].first << " : TEXCOORD" <<
            i << ";" << std::endl;
    }
    out << "};" << std::endl;
    out << "struct OutputData" << std::endl << "{" << std::endl;
    for(const auto& n : outputs)
    {
        out << "    " << n.second.second << " " << n.second.first << " : SV_Tar"
            "get" << n.first << ";" << std::endl;
    }
    if(usesGlFragDepth)
    {
        out << "    float glFragDepth : SV_Depth;" << std::endl;
    }
    out << "};" << std::endl;
    out << "OutputData main(InputData input)" << std::endl << "{" << std::endl;
    for(const auto& n : textures)
    {
        out << "    wrap_" << n.second.second << " " << n.first << " = {" << n.
            first << "Texture, " << n.first << "Sampler};" << std::endl;
    }
    out << "    OutputData output;" << std::endl << mainBuffer.str() << "    re"
        "turn output;" << std::endl << "}" << std::endl;

    return out.str();
}
