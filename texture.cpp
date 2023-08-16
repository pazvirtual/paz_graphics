#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "util.hpp"

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

paz::Texture::Texture() {}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, bool repeat,
    const void* data)
{
    init(width, height, numChannels, numBits, type, minFilter, magFilter,
        repeat, data);
}

void paz::Texture::init(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, bool repeat,
    const void* data)
{
    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    _internalFormat = internal_format(numChannels, numBits, type);
    const bool isInt = (type == DataType::UInt || type == DataType::SInt);
    if(numChannels == 1)
    {
        _format = isInt ? GL_RED_INTEGER : GL_RED;
    }
    else if(numChannels == 2)
    {
        _format = isInt ? GL_RG_INTEGER : GL_RG;
    }
    else if(numChannels == 4)
    {
        _format = isInt ? GL_RGBA_INTEGER : GL_RGBA;
    }
    else
    {
        throw std::runtime_error("Texture must have 1, 2, or 4 channels.");
    }

    _type = gl_type(type);

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _format,
        _type, data);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (repeat ? GL_REPEAT :
        GL_CLAMP_TO_EDGE));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (repeat ? GL_REPEAT :
        GL_CLAMP_TO_EDGE));
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, const std::
    array<GLfloat, 4>& border, const void* data)
{
    init(width, height, numChannels, numBits, type, minFilter, magFilter,
        border, data);
}

void paz::Texture::init(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, const std::
    array<GLfloat, 4>& border, const void* data)
{
    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    _internalFormat = internal_format(numChannels, numBits, type);
    const bool isInt = (type == DataType::UInt || type == DataType::SInt);
    if(numChannels == 1)
    {
        _format = isInt ? GL_RED_INTEGER : GL_RED;
    }
    else if(numChannels == 2)
    {
        _format = isInt ? GL_RG_INTEGER : GL_RG;
    }
    else if(numChannels == 4)
    {
        _format = isInt ? GL_RGBA_INTEGER : GL_RGBA;
    }
    else
    {
        throw std::runtime_error("Texture must have 1, 2, or 4 channels.");
    }

    _type = gl_type(type);

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _format,
        _type, data);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border.data());
}

void paz::Texture::resize(GLsizei width, GLsizei height)
{
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _format,
        _type, nullptr);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

#endif
