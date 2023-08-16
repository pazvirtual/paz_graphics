#ifndef UTIL_METAL_HH
#define UTIL_METAL_HH

#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import <MetalKit/MetalKit.h>

namespace paz
{
    MTLPixelFormat pixel_format(unsigned int c, unsigned int b, Texture::
        DataType t);
}

#endif

#endif
