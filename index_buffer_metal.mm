#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.h"
#import "view_controller.h"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::IndexBuffer::~IndexBuffer()
{
    if(_data)
    {
        [(id<MTLBuffer>)_data setPurgeableState:MTLPurgeableStateEmpty];
        [(id<MTLBuffer>)_data release];
    }
}

paz::IndexBuffer::IndexBuffer(const std::vector<unsigned int>& indices)
{
    _numIndices = indices.size();
    _data = [DEVICE newBufferWithBytes:indices.data() length:sizeof(unsigned
        int)*_numIndices options:MTLStorageModeShared];
}

#endif
