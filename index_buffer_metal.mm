#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::IndexBuffer::~IndexBuffer()
{
    if(_data->_data)
    {
        [(id<MTLBuffer>)_data->_data setPurgeableState:MTLPurgeableStateEmpty];
        [(id<MTLBuffer>)_data->_data release];
    }
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    _data = std::make_unique<Data>();

    _numIndices = size;
    _data->_data = [DEVICE newBufferWithBytes:data length:sizeof(unsigned int)*
        size options:MTLStorageModeShared];
}

#endif
