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
    init(image.width(), image.height(), 1, 8, normalized ? DataType::UNorm :
        DataType::UInt, minFilter, magFilter, image.data());
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter) :
    Texture()
{
    init(width, height, numChannels, numBits, type, minFilter, magFilter,
        nullptr);
}

void paz::Texture::init(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, const void*
    data)
{
    _width = width;
    _height = height;

    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    _data->_internalFormat = internal_format(numChannels, numBits, type);
    const bool isInt = (type == DataType::UInt || type == DataType::SInt);
    if(numChannels == 1)
    {
        _data->_format = isInt ? GL_RED_INTEGER : GL_RED;
    }
    else if(numChannels == 2)
    {
        _data->_format = isInt ? GL_RG_INTEGER : GL_RG;
    }
    else if(numChannels == 4)
    {
        _data->_format = isInt ? GL_RGBA_INTEGER : GL_RGBA;
    }
    else
    {
        throw std::runtime_error("Texture must have 1, 2, or 4 channels.");
    }

    _data->_type = gl_type(type, numBits);

    glGenTextures(1, &_data->_id);
    glBindTexture(GL_TEXTURE_2D, _data->_id);
    glTexImage2D(GL_TEXTURE_2D, 0, _data->_internalFormat, _width, _height, 0,
        _data->_format, _data->_type, data);
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
    _width = width;
    _height = height;

    glBindTexture(GL_TEXTURE_2D, _data->_id);
    glTexImage2D(GL_TEXTURE_2D, 0, _data->_internalFormat, _width, _height, 0,
        _data->_format, _data->_type, nullptr);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

#endif
