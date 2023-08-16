#ifndef PAZ_GRAPHICS_SHADING_LANG_HPP
#define PAZ_GRAPHICS_SHADING_LANG_HPP

#include "PAZ_Graphics"
#include <unordered_set>

namespace paz
{
    enum class Mode
    {
        None,  // None of the following
        Main,  // In main function
        Sig,   // In signature of private function
        Fun,   // In private function
        Struct // In struct definition
    };

    std::string vert2metal(const std::string& src);
    std::string frag2metal(const std::string& src);
    std::string vert2hlsl(const std::string& src, std::vector<std::tuple<std::
        string, DataType, int, int>>& uniforms);
    std::string frag2hlsl(const std::string& src, std::vector<std::tuple<std::
        string, DataType, int, int>>& uniforms);
    std::string process_sig(const std::string& sig, std::unordered_set<std::
        string>& argNames);
}

#endif
