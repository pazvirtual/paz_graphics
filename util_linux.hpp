#ifndef PAZ_GRAPHICS_UTIL_OPENGL_HPP
#define PAZ_GRAPHICS_UTIL_OPENGL_HPP

#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"

namespace paz
{
    constexpr int GlMajorVersion = 4;
    constexpr int GlMinorVersion = 1;

    std::pair<int, int> min_mag_filter(MinMagFilter minFilter, MinMagFilter
        magFilter, MipmapFilter mipmapFilter);
    int gl_internal_format(TextureFormat format);
    unsigned int gl_format(TextureFormat format);
    unsigned int gl_type(TextureFormat format);
    std::string get_log(unsigned int id, bool isProgram);
    std::string gl_error(unsigned int error) noexcept;
    unsigned int gl_type(DataType type);
}

#endif

#endif
