#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::Texture::~Texture()
{
    if(_data->_id)
    {
        glDeleteTextures(1, &_data->_id);
    }
}

paz::Texture::Texture()
{
    _data = std::make_unique<Data>();
}

paz::Texture::Texture(const Image<std::uint8_t, 1>& image, MinMagFilter
    minFilter, MinMagFilter magFilter, bool normalized) : Texture()
{
    _width = image.width();
    _height = image.height();
    _data->_format = normalized ? Format::R8UNorm : Format::R8UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    init(image.data());
}

paz::Texture::Texture(int width, int height, Format format, MinMagFilter
    minFilter, MinMagFilter magFilter) : Texture()
{
    _width = width;
    _height = height;
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    init();
}

void paz::Texture::init(const void* data)
{
    const auto filters = min_mag_filter(_data->_minFilter, _data->_magFilter);
    const auto min = filters.first;
    const auto mag = filters.second;

    glGenTextures(1, &_data->_id);
    glBindTexture(GL_TEXTURE_2D, _data->_id);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format(_data->_format), _width,
        _height, 0, gl_format(_data->_format), gl_type(_data->_format), data);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void paz::Texture::resize(GLsizei width, GLsizei height)
{
    if(_data->_scale)
    {
        _width = _data->_scale*width;
        _height = _data->_scale*height;
        glBindTexture(GL_TEXTURE_2D, _data->_id);
        glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format(_data->_format),
            _width, _height, 0, gl_format(_data->_format), gl_type(_data->
            _format), nullptr);
        if(_mipmap)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
}

#endif
