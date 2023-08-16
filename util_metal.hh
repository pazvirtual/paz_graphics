#ifndef PAZ_GRAPHICS_UTIL_METAL_HH
#define PAZ_GRAPHICS_UTIL_METAL_HH

#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import <MetalKit/MetalKit.h>

namespace paz
{
    MTLPixelFormat pixel_format(TextureFormat format);
    int bytes_per_pixel(TextureFormat format);
    id<MTLSamplerState> create_sampler(MinMagFilter minFilter, MinMagFilter
        magFilter, MipmapFilter mipFilter, WrapMode wrapS, WrapMode wrapT);
    id<MTLDepthStencilState> depth_stencil_state(DepthTestMode mode);
}

#endif

#endif
