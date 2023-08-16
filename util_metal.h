#ifndef UTIL_METAL_H
#define UTIL_METAL_H

#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import <MetalKit/MetalKit.h>

namespace paz
{
    MTLPixelFormat pixel_format(unsigned int c, unsigned int b, TextureBase::
        DataType t);
}

#endif

#endif
