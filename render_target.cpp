#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget(TextureFormat format, MinMagFilter minFilter,
    MinMagFilter magFilter, MipmapFilter mipFilter, WrapMode wrapS, WrapMode
    wrapT) : RenderTarget(format, 1., minFilter, magFilter, mipFilter, wrapS,
    wrapT) {}

paz::RenderTarget::RenderTarget(TextureFormat format, double scale, MinMagFilter
    minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, WrapMode wrapS,
    WrapMode wrapT)
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

paz::RenderTarget::RenderTarget(TextureFormat format, int width, int height,
    MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter mipFilter,
    WrapMode wrapS, WrapMode wrapT)
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
