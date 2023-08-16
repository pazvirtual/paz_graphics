#ifndef OPENGL2METAL_HPP
#define OPENGL2METAL_HPP

#include <iostream>

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
    std::string process_sig(const std::string& sig);
}

#endif
