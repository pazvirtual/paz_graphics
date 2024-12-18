#include "detect_os.hpp"

#ifdef PAZ_MACOS

#import "util_macos.hh"
#import "app_delegate.hh"
#import "view_controller.hh"

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CASE0(a, b) case TextureFormat::a: return MTLPixelFormat##b;
#define CASE1(f, n, b) case TextureFormat::f: return n*b/8;
#define DEPTH(a) static const DepthStencilState a(DepthTestMode::a);
#define CASE2(a) case DepthTestMode::a: return a._state;

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
        CASE0(RGBA8UNorm_sRGB, RGBA8Unorm_sRGB)
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

        CASE0(BGRA8UNorm, BGRA8Unorm)
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
        CASE1(RGBA8UNorm_sRGB, 4, 8)
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

        CASE1(BGRA8UNorm, 4, 8)
    }

    throw std::runtime_error("Invalid texture format requested.");
}

id<MTLDepthStencilState> paz::depth_stencil_state(DepthTestMode mode)
{
    struct DepthStencilState
    {
        id<MTLDepthStencilState> _state;
        DepthStencilState(paz::DepthTestMode mode)
        {
            MTLDepthStencilDescriptor* depthStencilDescriptor =
                [[MTLDepthStencilDescriptor alloc] init];
            if(mode == paz::DepthTestMode::Disable)
            {
                [depthStencilDescriptor setDepthWriteEnabled:NO];
            }
            else
            {
                if(mode == paz::DepthTestMode::Never || mode == paz::
                    DepthTestMode::NeverNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionNever];
                }
                else if(mode == paz::DepthTestMode::Less || mode == paz::
                    DepthTestMode::LessNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionLess];
                }
                else if(mode == paz::DepthTestMode::Equal || mode == paz::
                    DepthTestMode::EqualNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionEqual];
                }
                else if(mode == paz::DepthTestMode::LessEqual || mode == paz::
                    DepthTestMode::LessEqualNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionLessEqual];
                }
                else if(mode == paz::DepthTestMode::Greater || mode == paz::
                    DepthTestMode::GreaterNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionGreater];
                }
                else if(mode == paz::DepthTestMode::NotEqual || mode == paz::
                    DepthTestMode::NotEqualNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionNotEqual];
                }
                else if(mode == paz::DepthTestMode::GreaterEqual || mode ==
                    paz::DepthTestMode::GreaterEqualNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionGreaterEqual];
                }
                else if(mode == paz::DepthTestMode::Always || mode == paz::
                    DepthTestMode::AlwaysNoMask)
                {
                    [depthStencilDescriptor setDepthCompareFunction:
                        MTLCompareFunctionAlways];
                }
                else
                {
                    throw std::runtime_error("Invalid depth testing function.");
                }
                if(mode >= paz::DepthTestMode::Never)
                {
                    [depthStencilDescriptor setDepthWriteEnabled:YES];
                }
            }
            _state = [DEVICE newDepthStencilStateWithDescriptor:
                depthStencilDescriptor];
            [depthStencilDescriptor release];
        }
        ~DepthStencilState()
        {
            if(_state)
            {
                [_state release];
            }
        }
    };

    DEPTH(NeverNoMask);
    DEPTH(LessNoMask);
    DEPTH(EqualNoMask);
    DEPTH(LessEqualNoMask);
    DEPTH(GreaterNoMask);
    DEPTH(NotEqualNoMask);
    DEPTH(GreaterEqualNoMask);
    DEPTH(AlwaysNoMask);
    DEPTH(Never);
    DEPTH(Less);
    DEPTH(Equal);
    DEPTH(LessEqual);
    DEPTH(Greater);
    DEPTH(NotEqual);
    DEPTH(GreaterEqual);
    DEPTH(Always);

    switch(mode)
    {
        CASE2(NeverNoMask);
        CASE2(LessNoMask);
        CASE2(EqualNoMask);
        CASE2(LessEqualNoMask);
        CASE2(GreaterNoMask);
        CASE2(NotEqualNoMask);
        CASE2(GreaterEqualNoMask);
        CASE2(AlwaysNoMask);
        CASE2(Never);
        CASE2(Less);
        CASE2(Equal);
        CASE2(LessEqual);
        CASE2(Greater);
        CASE2(NotEqual);
        CASE2(GreaterEqual);
        CASE2(Always);
        case DepthTestMode::Disable: return nil;
    }

    throw std::logic_error("Invalid depth testing function");
}

#endif
