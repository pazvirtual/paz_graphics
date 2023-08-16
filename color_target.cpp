#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::ColorTarget::ColorTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    Texture::init(_scale*Window::ViewportWidth(), _scale*Window::
        ViewportHeight(), numChannels, numBits, type, minFilter, magFilter,
        nullptr);
}

void paz::ColorTarget::resize(GLsizei width, GLsizei height)
{
    RenderTarget::resize(width, height);
}

#endif
