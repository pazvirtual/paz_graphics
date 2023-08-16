#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import <MetalKit/MetalKit.h>

paz::TextureBase::TextureBase() {}

paz::TextureBase::~TextureBase()
{
    if(_texture)
    {
        [(id<MTLTexture>)_texture release];
    }
    if(_sampler)
    {
        [(id<MTLSamplerState>)_sampler release];
    }
}

#endif
