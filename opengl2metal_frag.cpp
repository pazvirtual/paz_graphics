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

    std::size_t numOpen = 0;
    std::size_t numClose = 0;

    bool usesGlFragDepth = false;

    std::unordered_map<std::string, std::pair<std::string, bool>> buffers;
    std::unordered_map<std::string, std::string> textures;
    std::map<std::string, std::pair<std::string, bool>> inputs;
    std::map<unsigned int, std::pair<std::string, std::string>> outputs;

    // Include headers.
    out << "#include <metal_stdlib>" << std::endl << "#include <simd/simd.h>" <<
        std::endl << "using namespace metal;" << std::endl;

    // Define our Y-reversed sample function.
    out << "template<typename T>" << std::endl << "auto texture(thread const te"
        "xture1d<T>& tex, thread const sampler&" << std::endl << "    sampler, "
        "thread float u)" << std::endl << "{" << std::endl << "    return tex.s"
        "ample(sampler, u);" << std::endl << "}" << std::endl;
    out << "template<typename T>" << std::endl << "auto texture(thread const te"
        "xture2d<T>& tex, thread const sampler&" << std::endl << "    sampler, "
        "thread const float2& uv)" << std::endl << "{" << std::endl << "    ret"
        "urn tex.sample(sampler, float2(uv.x, 1. - uv.y));" << std::endl << "}"
        << std::endl;

    std::string line;
    std::size_t l = 0;
    while(std::getline(in, line))
    {
        ++l;

        // Clean up lines.
        line = std::regex_replace(line, std::regex("//.*$"), "");
        line = std::regex_replace(line, std::regex("\\s+$"), "");
        if(line.empty() || line.substr(0, 8) == "#version" || line.substr(0, 10)
            == "#extension")
        {
            continue;
        }

        // Check for macro definitions and other unsupported features.
        if(line.substr(0, 7) == "#define")
        {
            throw std::runtime_error("Line " + std::to_string(l) + ": User-defi"
                "ned macros are not supported.");
        }
        if(std::regex_match(line, std::regex(".*\\btexture[34]D\\b.*")))
        {
            throw std::runtime_error("Higher-dimensional textures are not suppo"
                "rted.");
        }

        // Keep macro conditionals.
        if(std::regex_match(line, std::regex("\\s*#((end)?if|else|ifn?def).*")))
        {
            out << line << std::endl;
            continue;
        }

        // Check for inputs and outputs out of scope.
        if(mode != Mode::Main)
        {
            if(std::regex_match(line, std::regex(".*\\bgl_FragDepth\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r outputs cannot be accessed outside of main function.");
            }
            for(const auto& n : outputs)
            {
                if(std::regex_match(line, std::regex(".*\\b" + n.second.first +
                    "\\b.*")))
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": S"
                        "hader outputs cannot be accessed outside of main funct"
                        "ion.");
                }
            }
            for(const auto& n : inputs)
            {
                if(std::regex_match(line, std::regex(".*\\b" + n.first +
                    "\\b.*")))
                {
                    throw std::runtime_error("Line " + std::to_string(l) + ": S"
                        "hader inputs cannot be accessed outside of main functi"
                        "on.");
                }
            }
        }

        // End private function.
        if(mode == Mode::Fun && line == "}")
        {
            out << line << std::endl;
            mode = Mode::None;
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
        line = std::regex_replace(line, std::regex("\\bsampler([12])D\\b"),
            "texture$1d<float>");
        line = std::regex_replace(line, std::regex("\\bisampler([12])D\\b"),
            "texture$1d<int>");
        line = std::regex_replace(line, std::regex("\\busampler([12])D\\b"),
            "texture$1d<uint>");
        line = std::regex_replace(line, std::regex("\\btexture\\(([^,]*),"),
            "texture($1, $1Sampler,");

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
                out << process_sig(sigBuffer.str()) << std::endl;
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
            mainBuffer << line << std::endl;
            continue;
        }

        // Process uniforms, inputs, and outputs.
        if(line.substr(0, 7) == "uniform")
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            std::string name = dec.substr(pos + 1);
            if(type.substr(0, 7) == "texture")
            {
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
        if(line.substr(0, 2) == "in")
        {
            const std::string dec = line.substr(3, line.size() - 4);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs[name] = {type, false};
            continue;
        }
        if(line.substr(0, 7) == "flat in")
        {
            const std::string dec = line.substr(8, line.size() - 9);
            const std::size_t pos = dec.find_last_of(' ');
            const std::string type = dec.substr(0, pos);
            const std::string name = dec.substr(pos + 1);
            inputs[name] = {type, true};
            continue;
        }
        if(line.substr(0, 6) == "layout")
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
        if(mode == Mode::None && std::regex_match(line, std::regex("\\s*[a-zA-Z"
            "_][a-zA-Z_0-9]*\\s+[a-zA-Z_][a-zA-Z_0-9]*\\(.*")))
        {
            // Check if the signature is complete.
            numOpen = std::count(line.begin(), line.end(), '(');
            numClose = std::count(line.begin(), line.end(), ')');
            if(numOpen == numClose)
            {
                mode = Mode::Fun;
                out << process_sig(line) << std::endl;
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
        "lPosition [[position]];" << std::endl;
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
        out << ", " << n.second << " " << n.first << " [[texture(" << t << ")]]"
            << ", sampler " << n.first << "Sampler [[sampler(" << t << ")]]";
        ++t;
    }
    out << ")" << std::endl << "{" << std::endl << "    OutputData out;" <<
        std::endl << mainBuffer.str() << "    return out;" << std::endl << "}"
        << std::endl;

    return out.str();
}
