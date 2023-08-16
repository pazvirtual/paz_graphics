#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "util.hpp"

#define CASE_STRING(x) case x: return #x;

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::RenderTarget::RenderTarget() {}

paz::RenderTarget::~RenderTarget()
{
    paz::Window::UnregisterTarget(this);
}

paz::RenderTarget::RenderTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    Texture::init(_scale*Window::Width(), _scale*Window::Height(), numChannels,
        numBits, type, minFilter, magFilter, nullptr);
    paz::Window::RegisterTarget(this);
}

void paz::RenderTarget::resize(GLsizei width, GLsizei height)
{
    Texture::resize(_scale*width, _scale*height);
}

#endif
