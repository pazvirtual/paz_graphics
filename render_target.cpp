#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget(double scale, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter)
{
    if(scale <= 0.)
    {
        throw std::runtime_error("Render target scale must be positive.");
    }

    _data = std::make_shared<Data>();

    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_isRenderTarget = true;
    _data->_scale = scale;

    _data->_width = _data->_scale*Window::ViewportWidth();
    _data->_height = _data->_scale*Window::ViewportHeight();

    init();

    register_target(_data.get());
}

paz::RenderTarget::RenderTarget(int width, int height, TextureFormat format,
    MinMagFilter minFilter, MinMagFilter magFilter)
{
    _data = std::make_shared<Data>();

    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_isRenderTarget = true;
    _data->_scale = 0.;

    _data->_width = width;
    _data->_height = height;

    init();

    register_target(_data.get());
}
