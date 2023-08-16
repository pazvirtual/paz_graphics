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

paz::Texture::Texture()
{
    initialize();
}

paz::Texture::Texture(const Image<std::uint8_t, 1>& image, MinMagFilter
    minFilter, MinMagFilter magFilter, bool normalized)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = image.width();
    _data->_height = image.height();
    _data->_format = normalized ? TextureFormat::R8UNorm : TextureFormat::
        R8UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->init(image.data());
}

paz::Texture::Texture(int width, int height, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = width;
    _data->_height = height;
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->init();
}

void paz::Texture::Data::init(const void* data)
{
    const auto filters = min_mag_filter(_minFilter, _magFilter);
    const auto min = filters.first;
    const auto mag = filters.second;

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format(_format), _width, _height,
        0, gl_format(_format), gl_type(_format), data);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
        if(_mipmap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
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
