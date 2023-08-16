#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget(double scale, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode wrapT,
    MipmapFilter mipFilter)
{
    if(scale <= 0.)
    {
        throw std::runtime_error("Render target scale must be positive.");
    }

    _data = std::make_shared<Data>();

    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->_isRenderTarget = true;
    _data->_scale = scale;

    _data->_width = _data->_scale*Window::ViewportWidth();
    _data->_height = _data->_scale*Window::ViewportHeight();

    _data->init();

    register_target(_data.get());
}

paz::RenderTarget::RenderTarget(int width, int height, TextureFormat format,
    MinMagFilter minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode
    wrapT, MipmapFilter mipFilter)
{
    _data = std::make_shared<Data>();

    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->_isRenderTarget = true;
    _data->_scale = 0.;

    _data->_width = width;
    _data->_height = height;

    _data->init();

    register_target(_data.get());
}
