#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "util.hpp"

#define CASE_STRING(x) case x: return #x;

std::pair<GLint, GLint> paz::min_mag_filter(Texture::MinMagFilter minFilter,
    Texture::MinMagFilter magFilter, Texture::MipmapFilter mipmapFilter)
{
    GLint min;
    if(mipmapFilter == Texture::MipmapFilter::Nearest)
    {
        if(minFilter == Texture::MinMagFilter::Nearest)
        {
            min = GL_NEAREST_MIPMAP_NEAREST;
        }
        else if(minFilter == Texture::MinMagFilter::Linear)
        {
            min = GL_LINEAR_MIPMAP_NEAREST;
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else if(mipmapFilter == Texture::MipmapFilter::Linear)
    {
        if(minFilter == Texture::MinMagFilter::Nearest)
        {
            min = GL_NEAREST_MIPMAP_LINEAR;
        }
        else if(minFilter == Texture::MinMagFilter::Linear)
        {
            min = GL_LINEAR_MIPMAP_LINEAR;
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else if(mipmapFilter == Texture::MipmapFilter::None)
    {
        if(minFilter == Texture::MinMagFilter::Nearest)
        {
            min = GL_NEAREST;
        }
        else if(minFilter == Texture::MinMagFilter::Linear)
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
    if(magFilter == Texture::MinMagFilter::Nearest)
    {
        mag = GL_NEAREST;
    }
    else if(magFilter == Texture::MinMagFilter::Linear)
    {
        mag = GL_LINEAR;
    }
    else
    {
        throw std::runtime_error("Invalid magnification function.");
    }

    return {min, mag};
}

GLint paz::internal_format(int c, int b, Texture::DataType t)
{
    if(c != 1 && c != 2 && c != 4)
    {
        throw std::runtime_error("Texture must have 1, 2, or 4 channels.");
    }
    if(b != 8 && b != 16 && b != 32)
    {
        throw std::runtime_error("Texture must have 8, 16, or 32 bits per chann"
            "el.");
    }

    if(c == 1)
    {
        if(b == 8)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_R8UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_R8I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_R8;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_R8_SNORM;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_R16UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_R16I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_R16;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_R16_SNORM;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_R16F;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_R32UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_R32I;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_R32F;
            }
        }
    }
    else if(c == 2)
    {
        if(b == 8)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RG8UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RG8I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_RG8;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_RG8_SNORM;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RG16UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RG16I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_RG16;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_RG16_SNORM;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_RG16F;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RG32UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RG32I;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_RG32F;
            }
        }
    }
    else if(c == 4)
    {
        if(b == 8)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RGBA8UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RGBA8I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_RGBA8;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_RGBA8_SNORM;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RGBA16UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RGBA16I;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return GL_RGBA16;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return GL_RGBA16_SNORM;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_RGBA16F;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return GL_RGBA32UI;
            }
            else if(t == Texture::DataType::SInt)
            {
                return GL_RGBA32I;
            }
            else if(t == Texture::DataType::Float)
            {
                return GL_RGBA32F;
            }
        }
    }

    throw std::runtime_error("Invalid texture format requested.");
}

GLenum paz::gl_type(Texture::DataType t)
{
    if(t == Texture::DataType::UInt || t == Texture::DataType::UNorm)
    {
        return GL_UNSIGNED_INT;
    }
    else if(t == Texture::DataType::SInt || t == Texture::DataType::
        SNorm)
    {
        return GL_INT;
    }
    else if(t == Texture::DataType::Float)
    {
        return GL_FLOAT;
    }

    throw std::runtime_error("Invalid type requested.");
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

std::string paz::gl_error(GLenum error)
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
