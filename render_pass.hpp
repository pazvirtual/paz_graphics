#ifndef PAZ_GRAPHICS_RENDER_PASS_HPP
#define PAZ_GRAPHICS_RENDER_PASS_HPP

#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"

namespace paz
{
    void disable_blend_depth_cull();
}

#endif

#endif
