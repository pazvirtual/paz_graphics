#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

#define CASE(a, b) case paz::WrapMode::a: return GL_##b;

static GLint wrap_mode(paz::WrapMode m)
{
    switch(m)
    {
        CASE(Repeat, REPEAT)
        CASE(MirrorRepeat, MIRRORED_REPEAT)
        CASE(ClampToEdge, CLAMP_TO_EDGE)
        CASE(ClampToZero, CLAMP_TO_BORDER)
    }

    throw std::logic_error("Invalid texture wrapping mode requested.");
}

paz::Texture::Data::~Data()
{
    if(_isRenderTarget)
    {
        unregister_target(this);
    }

    if(_id)
    {
        glDeleteTextures(1, &_id);
    }
}

paz::Texture::Texture() {}

paz::Texture::Texture(const Image<std::uint8_t, 1>& image, MinMagFilter
    minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode wrapT,
    MipmapFilter mipFilter, bool normalized)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = image.width();
    _data->_height = image.height();
    _data->_format = normalized ? TextureFormat::R8UNorm : TextureFormat::
        R8UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->init(image.data());
}

paz::Texture::Texture(int width, int height, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode wrapT,
    MipmapFilter mipFilter)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = width;
    _data->_height = height;
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->init();
}

void paz::Texture::Data::init(const void* data)
{
    const auto filters = min_mag_filter(_minFilter, _magFilter, _mipFilter);
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format(_format), _width, _height,
        0, gl_format(_format), gl_type(_format), data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filters.first);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filters.second);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode(_wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode(_wrapT));
    ensureMipmaps();
}

void paz::Texture::Data::resize(int width, int height)
{
    if(_scale)
    {
        _width = _scale*width;
        _height = _scale*height;
        glBindTexture(GL_TEXTURE_2D, _id);
        glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format(_format), _width,
            _height, 0, gl_format(_format), gl_type(_format), nullptr);
        ensureMipmaps();
    }
}

void paz::Texture::Data::ensureMipmaps()
{
    if(_mipFilter != MipmapFilter::None)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void paz::Texture::resize(int width, int height)
{
    _data->resize(width, height);
}

int paz::Texture::width() const
{
    return _data->_width;
}

int paz::Texture::height() const
{
    return _data->_height;
}

#endif
