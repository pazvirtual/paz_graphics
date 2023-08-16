#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"

paz::ColorTarget::~ColorTarget()
{
    paz::Window::UnregisterTarget(this);
}

paz::ColorTarget::ColorTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    Texture::init(_scale*Window::ViewportWidth(), _scale*Window::
        ViewportHeight(), numChannels, numBits, type, minFilter, magFilter,
        nullptr);
    paz::Window::RegisterTarget(this);
}

paz::ColorTarget::resize(int width, int height)
{
    Texture::resize(_scale*width, _scale*height);
}

#endif
