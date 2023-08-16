#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"

#define CASE_STRING(x) case x: return #x;

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::RenderTarget::RenderTarget()
{
    _data = std::make_unique<Data>();
}

paz::RenderTarget::RenderTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter) :
    RenderTarget()
{
    _scale = scale;
    Texture::init(_scale*Window::ViewportWidth(), _scale*Window::
        ViewportHeight(), numChannels, numBits, type, minFilter, magFilter,
        nullptr);
    paz::Window::RegisterTarget(this);
}

paz::RenderTarget::~RenderTarget()
{
    paz::Window::UnregisterTarget(this);
}

void paz::RenderTarget::resize(GLsizei width, GLsizei height)
{
    Texture::resize(_scale*width, _scale*height);
}

#endif
