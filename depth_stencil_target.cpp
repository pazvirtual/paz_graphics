#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static GLint depth_internal_format(int b, paz::Texture::DataType t)
{
    if(b == 16)
    {
        if(t == paz::Texture::DataType::UNorm)
        {
            return GL_DEPTH_COMPONENT16;
        }
    }
    else if(b == 32)
    {
        if(t == paz::Texture::DataType::Float)
        {
            return GL_DEPTH_COMPONENT32F;
        }
    }

    throw std::runtime_error("Invalid depth texture format requested.");
}

paz::DepthStencilTarget::DepthStencilTarget(double scale, int numBits, DataType
    type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    float width = _scale*Window::ViewportWidth();
    float height = _scale*Window::ViewportHeight();

    _mipmap = false;//TEMP
    const auto filters = min_mag_filter(minFilter, magFilter/*, mipmapFilter*/);
    const auto min = filters.first;
    const auto mag = filters.second;

    Texture::_data->_internalFormat = depth_internal_format(numBits, type);
    Texture::_data->_format = GL_DEPTH_COMPONENT;

    Texture::_data->_type = gl_type(type);

    glGenTextures(1, &Texture::_data->_id);
    glBindTexture(GL_TEXTURE_2D, Texture::_data->_id);
    glTexImage2D(GL_TEXTURE_2D, 0, Texture::_data->_internalFormat, width,
        height, 0, Texture::_data->_format, Texture::_data->_type, nullptr);
    if(_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    paz::Window::RegisterTarget(this);
}

#endif
