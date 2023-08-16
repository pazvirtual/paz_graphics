#include "opengl2metal.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <regex>

static const std::vector<std::string> unsupportedBuiltins = {"gl_PerVertex",
    "gl_ClipDistance"};

std::string paz::vert2metal(const std::string& src)
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
    bool usesGlLineWidth = false;
    bool usesGlPointSize = false;

    std::unordered_map<std::string, std::pair<std::string, bool>> buffers;
    std::map<unsigned int, std::pair<std::string, std::string>> inputs;
    std::map<std::string, std::pair<std::string, bool>> outputs;

    // Include headers.
    out << "#include <metal_stdlib>" << std::endl << "#include <simd/simd.h>" <<
        std::endl << "using namespace metal;" << std::endl;

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
                "supported.");
        }
        if(std::regex_match(line, std::regex(".*\\b(float|u?int|[iu]?vec[2-4])"
            "\\s*\\[\\s*[a-zA-Z_0-9]*\\s*\\]\\s*\\(.*")))
        {
            throw std::runtime_error("Only braced initializer lists are support"
                "ed for array definition.");
        }
        if(std::regex_match(line, std::regex(".*\\b(float|u?int|[iu]?vec[2-4])"
            "\\s*\\[.*")))
        {
            throw std::runtime_error("Array dimensions must follow variable nam"
                "e.");
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

        // Check for inputs and outputs out of scope.
        if(mode != Mode::Main)
        {
            if(std::regex_match(line, std::regex(".*\\bgl_Position\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r outputs cannot be accessed outside of main function.");
            }
            if(std::regex_match(line, std::regex(".*\\bgl_VertexID\\b.*")))
            {
                throw std::runtime_error("Line " + std::to_string(l) + ": Shade"
                    "r inputs cannot be accessed outside of main function.");
            }
            if(!isFun && mode != Mode::Sig)
            {
                for(const auto& n : outputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.first + "\\b.*")) && !curArgNames.count(n.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) + ": Shader outputs cannot be accessed outside of main function.");
                    }
                }
                for(const auto& n : inputs)
                {
                    if(std::regex_match(line, std::regex(".*\\b" + n.second.first + "\\b.*")) && !curArgNames.count(n.second.first))
                    {
                        throw std::runtime_error("Line " + std::to_string(l) + ": Shader inputs cannot be accessed outside of main function.");
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
                line = std::regex_replace(line, std::regex("\\b" + n.second.
                    first + "\\b"), "in." + n.second.first);
            }
            for(const auto& n : outputs)
            {
                line = std::regex_replace(line, std::regex("\\b" + n.first +
                    "\\b"), "out." + n.first);
            }
            if(!usesGlPosition && std::regex_match(line, std::regex(".*\\bgl_Po"
                "sition\\b.*")))
            {
                usesGlPosition = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_Position\\b"),
                "out.glPosition");
            if(!usesGlVertexId && std::regex_match(line, std::regex(".*\\bgl_Ve"
                "rtexID\\b.*")))
            {
                usesGlVertexId = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_VertexID\\b"),
                "glVertexId");
            if(!usesGlLineWidth && std::regex_match(line, std::regex(".*\\bgl_L"
                "ineWidth\\b.*")))
            {
                usesGlLineWidth = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_LineWidth\\b"),
                "glLineWidth");
            if(!usesGlPointSize && std::regex_match(line, std::regex(".*\\bgl_P"
                "ointSize\\b.*")))
            {
                usesGlPointSize = true;
            }
            line = std::regex_replace(line, std::regex("\\bgl_PointSize\\b"),
                "out.glPointSize");
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
            const bool isPointer = (name.back() == ']');
            if(isPointer)
            {
                name = name.substr(0, name.find('['));
            }
            buffers[name] = {type, isPointer};
            continue;
        }
        if(std::regex_match(line, std::regex("layout\\b.*")))
        {
            const std::string layout = std::regex_replace(line, std::regex("^.*"
                "location\\s*=\\s*([0-9]+).*$"), "$1");
            const int i = std::stoi(layout);
            const std::string dec = std::regex_replace(line, std::regex("^.*in"
                "\\s+(.*);$"), "$1");
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

    if(!usesGlPosition)
    {
        throw std::runtime_error("Vertex position not set in shader.");
    }

    // Append main function.
    out << "struct InputData" << std::endl << "{" << std::endl;
    for(const auto& n : inputs)
    {
        out << "    " << n.second.second << " " << n.second.first <<
            " [[attribute(" << n.first << ")]];" << std::endl;
    }
    out << "};" << std::endl;
    out << "struct OutputData" << std::endl << "{" << std::endl << "    float4 "
        "glPosition [[position]];" << std::endl;
    if(usesGlPointSize)
    {
        out << "    float glPointSize [[point_size]];" << std::endl;
    }
    for(const auto& n : outputs)
    {
        out << "    " << n.second.first << " " << n.first << (n.second.second ?
            " [[flat]]" : "") << ";" << std::endl;
    }
    out << "};" << std::endl;
    out << "vertex OutputData vertMain(InputData in [[stage_in]]";
    if(usesGlVertexId)
    {
        out << ", uint glVertexId [[vertex_id]]";
    }
    int b = inputs.size();
    for(const auto& n : buffers)
    {
        out << ", constant " << n.second.first << (n.second.second ? "* " :
            "& ") << n.first << " [[buffer(" << b << ")]]";
        ++b;
    }
    out << ")" << std::endl << "{" << std::endl << "    OutputData out;" <<
        std::endl;
    if(usesGlLineWidth)
    {
        out << "    float glLineWidth;" << std::endl;
    }
    out << mainBuffer.str() << "    out.glPosition = float4x4(1, 0, 0, 0, 0, 1,"
        " 0, 0, 0, 0, 0.5, 0, 0, 0, 0.5," << std::endl << "        1)*out.glPos"
        "ition;" << std::endl << "    return out;" << std::endl << "}" << std::
        endl;

    return out.str();
}
