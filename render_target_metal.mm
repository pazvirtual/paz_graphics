#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget(double scale, Format format, MinMagFilter
    minFilter, MinMagFilter magFilter) : Texture()
{
    if(scale <= 0.)
    {
        throw std::runtime_error("Render target scale must be positive.");
    }

    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_isRenderTarget = true;
    _data->_scale = scale;

    _width = _data->_scale*Window::ViewportWidth();
    _height = _data->_scale*Window::ViewportHeight();

    init();

    register_target(this);
}

paz::RenderTarget::RenderTarget(int width, int height, Format format,
    MinMagFilter minFilter, MinMagFilter magFilter) : Texture()
{
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_isRenderTarget = true;
    _data->_scale = 0.;

    _width = width;
    _height = height;

    init();

    register_target(this);
}

paz::RenderTarget::~RenderTarget()
{
    unregister_target(this);
}

#endif
