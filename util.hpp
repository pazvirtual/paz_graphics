#ifndef PAZ_GRAPHICS_UTIL_HPP
#define PAZ_GRAPHICS_UTIL_HPP

#include "detect_os.hpp"

#ifndef PAZ_MACOS

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
}

#endif

#endif
