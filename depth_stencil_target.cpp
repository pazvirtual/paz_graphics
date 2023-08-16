#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "util.hpp"

#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static GLint depth_internal_format(int b, paz::TextureBase::DataType t)
{
    if(b == 16)
    {
        if(t == paz::TextureBase::DataType::UNorm)
        {
            return GL_DEPTH_COMPONENT16;
        }
    }
    else if(b == 32)
    {
        if(t == paz::TextureBase::DataType::Float)
        {
            return GL_DEPTH_COMPONENT32F;
        }
    }

    throw std::runtime_error("Invalid depth texture format requested.");
}

paz::DepthStencilTarget::DepthStencilTarget(double scale, int numBits, DataType
    type, MinMagFilter minFilter, MinMagFilter magFilter, bool repeat)
{
    _scale = scale;
    float width = _scale*Window::Width();
    float height = _scale*Window::Height();

    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    _internalFormat = depth_internal_format(numBits, type);
    _format = GL_DEPTH_COMPONENT;

    _type = gl_type(type);

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _format,
        _type, nullptr);
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

    paz::Window::RegisterTarget(this);
}

paz::DepthStencilTarget::DepthStencilTarget(double scale, int numBits, DataType
    type, MinMagFilter minFilter, MinMagFilter magFilter, const std::array<
    float, 4>& border)
{
    _scale = scale;
    float width = _scale*Window::Width();
    float height = _scale*Window::Height();

    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    _internalFormat = depth_internal_format(numBits, type);
    _format = GL_DEPTH_COMPONENT;

    _type = gl_type(type);

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _format,
        _type, nullptr);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border.data());

    paz::Window::RegisterTarget(this);
}

#endif
