#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include "gl_core_4_1.h"
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

paz::Texture::Texture()
{
    initialize();
}

#define TEX(t, n, f) paz::Texture::Texture(const Image<t, n>& image, \
    MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, \
    WrapMode wrapS, WrapMode wrapT)\
{\
    initialize();\
    _data = std::make_shared<Data>();\
    _data->_width = image.width();\
    _data->_height = image.height();\
    _data->_format = TextureFormat::f;\
    _data->_minFilter = minFilter;\
    _data->_magFilter = magFilter;\
    _data->_mipFilter = mipFilter;\
    _data->_wrapS = wrapS;\
    _data->_wrapT = wrapT;\
    _data->init(image.data());\
}

#define TEX_NORM(t, n, f) paz::Texture::Texture(const Image<t, n>& image, \
    MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, \
    WrapMode wrapS, WrapMode wrapT, bool normalized)\
{\
    initialize();\
    _data = std::make_shared<Data>();\
    _data->_width = image.width();\
    _data->_height = image.height();\
    _data->_format = normalized ? TextureFormat::f##Norm : TextureFormat::\
        f##Int;\
    _data->_minFilter = minFilter;\
    _data->_magFilter = magFilter;\
    _data->_mipFilter = mipFilter;\
    _data->_wrapS = wrapS;\
    _data->_wrapT = wrapT;\
    _data->init(image.data());\
}

TEX_NORM(std::int8_t, 1, R8S)
TEX_NORM(std::int8_t, 2, RG8S)
TEX_NORM(std::int8_t, 4, RGBA8S)
TEX_NORM(std::int16_t, 1, R16S)
TEX_NORM(std::int16_t, 2, RG16S)
TEX_NORM(std::int16_t, 4, RGBA16S)
TEX(std::int32_t, 1, R32SInt)
TEX(std::int32_t, 2, RG32SInt)
TEX(std::int32_t, 4, RGBA32SInt)
TEX_NORM(std::uint8_t, 1, R8U)
TEX_NORM(std::uint8_t, 2, RG8U)
TEX_NORM(std::uint8_t, 4, RGBA8U)
TEX_NORM(std::uint16_t, 1, R16U)
TEX_NORM(std::uint16_t, 2, RG16U)
TEX_NORM(std::uint16_t, 4, RGBA16U)
TEX(std::uint32_t, 1, R32UInt)
TEX(std::uint32_t, 2, RG32UInt)
TEX(std::uint32_t, 4, RGBA32UInt)
TEX(float, 1, R32Float)
TEX(float, 2, RG32Float)
TEX(float, 4, RGBA32Float)

paz::Texture::Texture(int width, int height, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, WrapMode wrapS,
    WrapMode wrapT)
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

paz::Texture::Texture(RenderTarget&& target) : _data(std::move(target._data)) {}

void paz::Texture::Data::init(const void* data)
{
    // This is because of Metal restrictions.
    if((_format == TextureFormat::Depth16UNorm || _format == TextureFormat::
        Depth32Float) && _mipFilter != MipmapFilter::None)
    {
        throw std::runtime_error("Depth/stencil textures do not support mipmapp"
            "ing.");
    }
    const auto filters = min_mag_filter(_minFilter, _magFilter, _mipFilter);
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
        glBindTexture(GL_TEXTURE_2D, _id);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
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
