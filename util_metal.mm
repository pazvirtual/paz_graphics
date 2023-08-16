#include "detect_os.hpp"

#ifdef PAZ_MACOS

#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static MTLSamplerMinMagFilter min_mag_filter(paz::Texture::MinMagFilter f)
{
    switch(f)
    {
        case paz::Texture::MinMagFilter::Linear: return
            MTLSamplerMinMagFilterLinear;
        case paz::Texture::MinMagFilter::Nearest: return
            MTLSamplerMinMagFilterNearest;
    }

    throw std::logic_error("Invalid texture filter requested.");
}

MTLPixelFormat paz::pixel_format(unsigned int c, unsigned int b, Texture::
    DataType t)
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
                return MTLPixelFormatR8Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatR8Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatR8Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatR8Snorm;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatR16Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatR16Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatR16Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatR16Snorm;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatR16Float;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatR32Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatR32Sint;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatR32Float;
            }
        }
    }
    else if(c == 2)
    {
        if(b == 8)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRG8Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRG8Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatRG8Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatRG8Snorm;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRG16Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRG16Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatRG16Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatRG16Snorm;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatRG16Float;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRG32Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRG32Sint;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatRG32Float;
            }
        }
    }
    else if(c == 4)
    {
        if(b == 8)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRGBA8Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRGBA8Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatRGBA8Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatRGBA8Snorm;
            }
        }
        else if(b == 16)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRGBA16Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRGBA16Sint;
            }
            else if(t == Texture::DataType::UNorm)
            {
                return MTLPixelFormatRGBA16Unorm;
            }
            else if(t == Texture::DataType::SNorm)
            {
                return MTLPixelFormatRGBA16Snorm;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatRGBA16Float;
            }
        }
        else if(b == 32)
        {
            if(t == Texture::DataType::UInt)
            {
                return MTLPixelFormatRGBA32Uint;
            }
            else if(t == Texture::DataType::SInt)
            {
                return MTLPixelFormatRGBA32Sint;
            }
            else if(t == Texture::DataType::Float)
            {
                return MTLPixelFormatRGBA32Float;
            }
        }
    }

    throw std::runtime_error("Invalid texture format requested.");
}

id<MTLSamplerState> paz::create_sampler(Texture::MinMagFilter minFilter,
    Texture::MinMagFilter magFilter)
{
    MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
    [descriptor setMinFilter:min_mag_filter(minFilter)];
    [descriptor setMagFilter:min_mag_filter(magFilter)];
    [descriptor setSAddressMode:MTLSamplerAddressModeRepeat];
    [descriptor setTAddressMode:MTLSamplerAddressModeRepeat];
    id<MTLSamplerState> sampler = [DEVICE newSamplerStateWithDescriptor:
        descriptor];
    [descriptor release];
    return sampler;
}

#endif
