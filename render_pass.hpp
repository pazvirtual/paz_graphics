#ifndef PAZ_GRAPHICS_RENDER_PASS_HPP
#define PAZ_GRAPHICS_RENDER_PASS_HPP

#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"

namespace paz
{
    void disable_blend_depth_cull();
}

#endif

#endif
