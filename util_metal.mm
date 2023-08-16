#include "detect_os.hpp"

#ifdef PAZ_MACOS

#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CASE(a, b) case Texture::Format::a: return MTLPixelFormat##b;
#define CASE1(f, n, b) case Texture::Format::f: return n*b/8;

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

MTLPixelFormat paz::pixel_format(Texture::Format format)
{
    switch(format)
    {
        CASE(R8UInt, R8Uint)
        CASE(R8SInt, R8Sint)
        CASE(R8UNorm, R8Unorm)
        CASE(R8SNorm, R8Snorm)
        CASE(R16UInt, R16Uint)
        CASE(R16SInt, R16Sint)
        CASE(R16UNorm, R16Unorm)
        CASE(R16SNorm, R16Snorm)
        CASE(R16Float, R16Float)
        CASE(R32UInt, R32Uint)
        CASE(R32SInt, R32Sint)
        CASE(R32Float, R32Float)

        CASE(RG8UInt, RG8Uint)
        CASE(RG8SInt, RG8Sint)
        CASE(RG8UNorm, RG8Unorm)
        CASE(RG8SNorm, RG8Snorm)
        CASE(RG16UInt, RG16Uint)
        CASE(RG16SInt, RG16Sint)
        CASE(RG16UNorm, RG16Unorm)
        CASE(RG16SNorm, RG16Snorm)
        CASE(RG16Float, RG16Float)
        CASE(RG32UInt, RG32Uint)
        CASE(RG32SInt, RG32Sint)
        CASE(RG32Float, RG32Float)

        CASE(RGBA8UInt, RGBA8Uint)
        CASE(RGBA8SInt, RGBA8Sint)
        CASE(RGBA8UNorm, RGBA8Unorm)
        CASE(RGBA8SNorm, RGBA8Snorm)
        CASE(RGBA16UInt, RGBA16Uint)
        CASE(RGBA16SInt, RGBA16Sint)
        CASE(RGBA16UNorm, RGBA16Unorm)
        CASE(RGBA16SNorm, RGBA16Snorm)
        CASE(RGBA16Float, RGBA16Float)
        CASE(RGBA32UInt, RGBA32Uint)
        CASE(RGBA32SInt, RGBA32Sint)
        CASE(RGBA32Float, RGBA32Float)

        CASE(Depth16UNorm, Depth16Unorm)
        CASE(Depth32Float, Depth32Float)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

int paz::bytes_per_pixel(Texture::Format format)
{
    switch(format)
    {
        CASE1(R8UInt, 1, 8)
        CASE1(R8SInt, 1, 8)
        CASE1(R8UNorm, 1, 8)
        CASE1(R8SNorm, 1, 8)
        CASE1(R16UInt, 1, 16)
        CASE1(R16SInt, 1, 16)
        CASE1(R16UNorm, 1, 16)
        CASE1(R16SNorm, 1, 16)
        CASE1(R16Float, 1, 16)
        CASE1(R32UInt, 1, 32)
        CASE1(R32SInt, 1, 32)
        CASE1(R32Float, 1, 32)

        CASE1(RG8UInt, 2, 8)
        CASE1(RG8SInt, 2, 8)
        CASE1(RG8UNorm, 2, 8)
        CASE1(RG8SNorm, 2, 8)
        CASE1(RG16UInt, 2, 16)
        CASE1(RG16SInt, 2, 16)
        CASE1(RG16UNorm, 2, 16)
        CASE1(RG16SNorm, 2, 16)
        CASE1(RG16Float, 2, 16)
        CASE1(RG32UInt, 2, 32)
        CASE1(RG32SInt, 2, 32)
        CASE1(RG32Float, 2, 32)

        CASE1(RGBA8UInt, 4, 8)
        CASE1(RGBA8SInt, 4, 8)
        CASE1(RGBA8UNorm, 4, 8)
        CASE1(RGBA8SNorm, 4, 8)
        CASE1(RGBA16UInt, 4, 16)
        CASE1(RGBA16SInt, 4, 16)
        CASE1(RGBA16UNorm, 4, 16)
        CASE1(RGBA16SNorm, 4, 16)
        CASE1(RGBA16Float, 4, 16)
        CASE1(RGBA32UInt, 4, 32)
        CASE1(RGBA32SInt, 4, 32)
        CASE1(RGBA32Float, 4, 32)

        CASE1(Depth16UNorm, 1, 16)
        CASE1(Depth32Float, 1, 32)
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
