#include "detect_os.hpp"

#ifdef PAZ_MACOS

#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CASE0(a, b) case TextureFormat::a: return MTLPixelFormat##b;
#define CASE1(f, n, b) case TextureFormat::f: return n*b/8;
#define CASE2(a, b) case paz::WrapMode::a: return MTLSamplerAddressMode##b;

static MTLSamplerMinMagFilter min_mag_filter(paz::MinMagFilter f)
{
    switch(f)
    {
        case paz::MinMagFilter::Linear: return MTLSamplerMinMagFilterLinear;
        case paz::MinMagFilter::Nearest: return MTLSamplerMinMagFilterNearest;
    }

    throw std::logic_error("Invalid texture filter requested.");
}

static MTLSamplerAddressMode address_mode(paz::WrapMode m)
{
    switch(m)
    {
        CASE2(Repeat, Repeat)
        CASE2(MirrorRepeat, MirrorRepeat)
        CASE2(ClampToEdge, ClampToEdge)
        CASE2(ClampToZero, ClampToZero)
    }

    throw std::logic_error("Invalid texture wrapping mode requested.");
}

MTLPixelFormat paz::pixel_format(TextureFormat format)
{
    switch(format)
    {
        CASE0(R8UInt, R8Uint)
        CASE0(R8SInt, R8Sint)
        CASE0(R8UNorm, R8Unorm)
        CASE0(R8SNorm, R8Snorm)
        CASE0(R16UInt, R16Uint)
        CASE0(R16SInt, R16Sint)
        CASE0(R16UNorm, R16Unorm)
        CASE0(R16SNorm, R16Snorm)
        CASE0(R16Float, R16Float)
        CASE0(R32UInt, R32Uint)
        CASE0(R32SInt, R32Sint)
        CASE0(R32Float, R32Float)

        CASE0(RG8UInt, RG8Uint)
        CASE0(RG8SInt, RG8Sint)
        CASE0(RG8UNorm, RG8Unorm)
        CASE0(RG8SNorm, RG8Snorm)
        CASE0(RG16UInt, RG16Uint)
        CASE0(RG16SInt, RG16Sint)
        CASE0(RG16UNorm, RG16Unorm)
        CASE0(RG16SNorm, RG16Snorm)
        CASE0(RG16Float, RG16Float)
        CASE0(RG32UInt, RG32Uint)
        CASE0(RG32SInt, RG32Sint)
        CASE0(RG32Float, RG32Float)

        CASE0(RGBA8UInt, RGBA8Uint)
        CASE0(RGBA8SInt, RGBA8Sint)
        CASE0(RGBA8UNorm, RGBA8Unorm)
        CASE0(RGBA8SNorm, RGBA8Snorm)
        CASE0(RGBA16UInt, RGBA16Uint)
        CASE0(RGBA16SInt, RGBA16Sint)
        CASE0(RGBA16UNorm, RGBA16Unorm)
        CASE0(RGBA16SNorm, RGBA16Snorm)
        CASE0(RGBA16Float, RGBA16Float)
        CASE0(RGBA32UInt, RGBA32Uint)
        CASE0(RGBA32SInt, RGBA32Sint)
        CASE0(RGBA32Float, RGBA32Float)

        CASE0(Depth16UNorm, Depth16Unorm)
        CASE0(Depth32Float, Depth32Float)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

int paz::bytes_per_pixel(TextureFormat format)
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

id<MTLSamplerState> paz::create_sampler(MinMagFilter minFilter, MinMagFilter
    magFilter, WrapMode wrapS, WrapMode wrapT)
{
    MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
    [descriptor setMinFilter:min_mag_filter(minFilter)];
    [descriptor setMagFilter:min_mag_filter(magFilter)];
    [descriptor setSAddressMode:address_mode(wrapS)];
    [descriptor setTAddressMode:address_mode(wrapT)];
    id<MTLSamplerState> sampler = [DEVICE newSamplerStateWithDescriptor:
        descriptor];
    [descriptor release];
    return sampler;
}

#endif
