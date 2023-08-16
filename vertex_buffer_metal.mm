#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "window.hpp"
#include "internal_data.hpp"
#include "vertex_buffer.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

paz::VertexBuffer::Data::~Data()
{
    for(auto& n : _buffers)
    {
        if(n)
        {
            [static_cast<id<MTLBuffer>>(n) setPurgeableState:
                MTLPurgeableStateEmpty];
            [static_cast<id<MTLBuffer>>(n) release];
        }
    }
}

paz::VertexBuffer::Data::Data() {}

paz::VertexBuffer::VertexBuffer()
{
    initialize();

    _data = std::make_unique<Data>();
}

void paz::VertexBuffer::attribute(int dim, const float* data, std::size_t size)
{
    check_size(dim, _data->_numVertices, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        float)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const unsigned int* data, std::size_t
    size)
{
    check_size(dim, _data->_numVertices, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        unsigned int)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const int* data, std::size_t size)
{
    check_size(dim, _data->_numVertices, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        int)*size options:MTLStorageModeShared]);
}

#endif
