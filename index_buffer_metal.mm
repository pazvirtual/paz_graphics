#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#include "window.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

paz::IndexBuffer::Data::~Data()
{
    if(_data)
    {
        [static_cast<id<MTLBuffer>>(_data) setPurgeableState:
            MTLPurgeableStateEmpty];
        [static_cast<id<MTLBuffer>>(_data) release];
    }
}

paz::IndexBuffer::IndexBuffer() {}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    _data->_data = [DEVICE newBufferWithBytes:data length:sizeof(unsigned int)*
        size options:MTLStorageModeShared];
}

#endif
