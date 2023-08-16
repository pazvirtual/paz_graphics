#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"
#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>

#define CASE_STRING(x) case x: return #x;
#define CASE(a, b) case TextureFormat::a: return GL_##b;

std::pair<GLint, GLint> paz::min_mag_filter(MinMagFilter minFilter, MinMagFilter
    magFilter, MipmapFilter mipmapFilter)
{
    GLint min;
    if(mipmapFilter == MipmapFilter::Nearest)
    {
        if(minFilter == MinMagFilter::Nearest)
        {
            min = GL_NEAREST_MIPMAP_NEAREST;
        }
        else if(minFilter == MinMagFilter::Linear)
        {
            min = GL_LINEAR_MIPMAP_NEAREST;
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else if(mipmapFilter == MipmapFilter::Linear)
    {
        if(minFilter == MinMagFilter::Nearest)
        {
            min = GL_NEAREST_MIPMAP_LINEAR;
        }
        else if(minFilter == MinMagFilter::Linear)
        {
            min = GL_LINEAR_MIPMAP_LINEAR;
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else if(mipmapFilter == MipmapFilter::None)
    {
        if(minFilter == MinMagFilter::Nearest)
        {
            min = GL_NEAREST;
        }
        else if(minFilter == MinMagFilter::Linear)
        {
            min = GL_LINEAR;
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else
    {
        throw std::runtime_error("Invalid mipmap selection function.");
    }

    GLint mag;
    if(magFilter == MinMagFilter::Nearest)
    {
        mag = GL_NEAREST;
    }
    else if(magFilter == MinMagFilter::Linear)
    {
        mag = GL_LINEAR;
    }
    else
    {
        throw std::runtime_error("Invalid magnification function.");
    }

    return {min, mag};
}

GLint paz::gl_internal_format(TextureFormat format)
{
    switch(format)
    {
        CASE(R8UInt, R8UI)
        CASE(R8SInt, R8I)
        CASE(R8UNorm, R8)
        CASE(R8SNorm, R8_SNORM)
        CASE(R16UInt, R16UI)
        CASE(R16SInt, R16I)
        CASE(R16UNorm, R16)
        CASE(R16SNorm, R16_SNORM)
        CASE(R16Float, R16F)
        CASE(R32UInt, R32UI)
        CASE(R32SInt, R32I)
        CASE(R32Float, R32F)

        CASE(RG8UInt, RG8UI)
        CASE(RG8SInt, RG8I)
        CASE(RG8UNorm, RG8)
        CASE(RG8SNorm, RG8_SNORM)
        CASE(RG16UInt, RG16UI)
        CASE(RG16SInt, RG16I)
        CASE(RG16UNorm, RG16)
        CASE(RG16SNorm, RG16_SNORM)
        CASE(RG16Float, RG16F)
        CASE(RG32UInt, RG32UI)
        CASE(RG32SInt, RG32I)
        CASE(RG32Float, RG32F)

        CASE(RGBA8UInt, RGBA8UI)
        CASE(RGBA8SInt, RGBA8I)
        CASE(RGBA8UNorm, RGBA8)
        CASE(RGBA8SNorm, RGBA8_SNORM)
        CASE(RGBA16UInt, RGBA16UI)
        CASE(RGBA16SInt, RGBA16I)
        CASE(RGBA16UNorm, RGBA16)
        CASE(RGBA16SNorm, RGBA16_SNORM)
        CASE(RGBA16Float, RGBA16F)
        CASE(RGBA32UInt, RGBA32UI)
        CASE(RGBA32SInt, RGBA32I)
        CASE(RGBA32Float, RGBA32F)

        CASE(Depth16UNorm, DEPTH_COMPONENT16)
        CASE(Depth32Float, DEPTH_COMPONENT32F)

        CASE(BGRA8UNorm, RGBA8)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

GLenum paz::gl_format(TextureFormat format)
{
    switch(format)
    {
        CASE(R8UInt, RED_INTEGER)
        CASE(R8SInt, RED_INTEGER)
        CASE(R8UNorm, RED)
        CASE(R8SNorm, RED)
        CASE(R16UInt, RED_INTEGER)
        CASE(R16SInt, RED_INTEGER)
        CASE(R16UNorm, RED)
        CASE(R16SNorm, RED)
        CASE(R16Float, RED)
        CASE(R32UInt, RED_INTEGER)
        CASE(R32SInt, RED_INTEGER)
        CASE(R32Float, RED)

        CASE(RG8UInt, RG_INTEGER)
        CASE(RG8SInt, RG_INTEGER)
        CASE(RG8UNorm, RG)
        CASE(RG8SNorm, RG)
        CASE(RG16UInt, RG_INTEGER)
        CASE(RG16SInt, RG_INTEGER)
        CASE(RG16UNorm, RG)
        CASE(RG16SNorm, RG)
        CASE(RG16Float, RG)
        CASE(RG32UInt, RG_INTEGER)
        CASE(RG32SInt, RG_INTEGER)
        CASE(RG32Float, RG)

        CASE(RGBA8UInt, RGBA_INTEGER)
        CASE(RGBA8SInt, RGBA_INTEGER)
        CASE(RGBA8UNorm, RGBA)
        CASE(RGBA8SNorm, RGBA)
        CASE(RGBA16UInt, RGBA_INTEGER)
        CASE(RGBA16SInt, RGBA_INTEGER)
        CASE(RGBA16UNorm, RGBA)
        CASE(RGBA16SNorm, RGBA)
        CASE(RGBA16Float, RGBA)
        CASE(RGBA32UInt, RGBA_INTEGER)
        CASE(RGBA32SInt, RGBA_INTEGER)
        CASE(RGBA32Float, RGBA)

        CASE(Depth16UNorm, DEPTH_COMPONENT)
        CASE(Depth32Float, DEPTH_COMPONENT)

        CASE(BGRA8UNorm, BGRA)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

GLenum paz::gl_type(TextureFormat format)
{
    switch(format)
    {
        CASE(R8UInt, UNSIGNED_BYTE)
        CASE(R8SInt, BYTE)
        CASE(R8UNorm, UNSIGNED_BYTE)
        CASE(R8SNorm, BYTE)
        CASE(R16UInt, UNSIGNED_SHORT)
        CASE(R16SInt, SHORT)
        CASE(R16UNorm, UNSIGNED_SHORT)
        CASE(R16SNorm, SHORT)
        CASE(R16Float, HALF_FLOAT) // Note: C++ does not have a `half` type.
        CASE(R32UInt, UNSIGNED_INT)
        CASE(R32SInt, INT)
        CASE(R32Float, FLOAT)

        CASE(RG8UInt, UNSIGNED_BYTE)
        CASE(RG8SInt, BYTE)
        CASE(RG8UNorm, UNSIGNED_BYTE)
        CASE(RG8SNorm, BYTE)
        CASE(RG16UInt, UNSIGNED_SHORT)
        CASE(RG16SInt, SHORT)
        CASE(RG16UNorm, UNSIGNED_SHORT)
        CASE(RG16SNorm, SHORT)
        CASE(RG16Float, HALF_FLOAT) // Note: C++ does not have a `half` type.
        CASE(RG32UInt, UNSIGNED_INT)
        CASE(RG32SInt, INT)
        CASE(RG32Float, FLOAT)

        CASE(RGBA8UInt, UNSIGNED_BYTE)
        CASE(RGBA8SInt, BYTE)
        CASE(RGBA8UNorm, UNSIGNED_BYTE)
        CASE(RGBA8SNorm, BYTE)
        CASE(RGBA16UInt, UNSIGNED_SHORT)
        CASE(RGBA16SInt, SHORT)
        CASE(RGBA16UNorm, UNSIGNED_SHORT)
        CASE(RGBA16SNorm, SHORT)
        CASE(RGBA16Float, HALF_FLOAT) // Note: C++ does not have a `half` type.
        CASE(RGBA32UInt, UNSIGNED_INT)
        CASE(RGBA32SInt, INT)
        CASE(RGBA32Float, FLOAT)

        CASE(Depth16UNorm, UNSIGNED_SHORT)
        CASE(Depth32Float, FLOAT)

        CASE(BGRA8UNorm, UNSIGNED_BYTE)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

std::string paz::get_log(unsigned int id, bool isProgram)
{
    int logLen = 0;
    if(isProgram)
    {
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLen);
    }
    else
    {
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
    }

    if(!logLen)
    {
        throw std::runtime_error("No log returned by shader " + std::string(
            isProgram ? "linker" : "compiler") + " after failure");
    }

    std::vector<char> buf(logLen);
    if(isProgram)
    {
        glGetProgramInfoLog(id, logLen, &logLen, buf.data());
    }
    else
    {
        glGetShaderInfoLog(id, logLen, &logLen, buf.data());
    }

    return std::string(buf.begin(), buf.begin() + logLen);
}

std::string paz::gl_error(GLenum error) noexcept
{
    switch(error)
    {
        CASE_STRING(GL_NO_ERROR)
        CASE_STRING(GL_INVALID_ENUM)
        CASE_STRING(GL_INVALID_VALUE)
        CASE_STRING(GL_INVALID_OPERATION)
        CASE_STRING(GL_INVALID_FRAMEBUFFER_OPERATION)
        CASE_STRING(GL_OUT_OF_MEMORY)
#ifdef GL_STACK_UNDERFLOW
        CASE_STRING(GL_STACK_UNDERFLOW)
#endif
#ifdef GL_STACK_OVERFLOW
        CASE_STRING(GL_STACK_OVERFLOW)
#endif
        default: return "Unrecognized OpenGL error code " + std::to_string(
            error);
    }
}

#endif
