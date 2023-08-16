#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget()
{
    _data = std::make_unique<Data>();
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
