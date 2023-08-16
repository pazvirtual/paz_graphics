#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import <MetalKit/MetalKit.h>

namespace paz
{
    MTLPixelFormat pixel_format(TextureFormat format);
    int bytes_per_pixel(TextureFormat format);
    id<MTLDepthStencilState> depth_stencil_state(DepthTestMode mode);
}

#endif
