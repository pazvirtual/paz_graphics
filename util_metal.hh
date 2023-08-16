#ifndef PAZ_GRAPHICS_UTIL_METAL_HH
#define PAZ_GRAPHICS_UTIL_METAL_HH

#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import <MetalKit/MetalKit.h>

namespace paz
{
    MTLPixelFormat pixel_format(unsigned int c, unsigned int b, Texture::
        DataType t);
    id<MTLSamplerState> create_sampler(paz::Texture::MinMagFilter minFilter,
        paz::Texture::MinMagFilter magFilter);
}

#endif

#endif
