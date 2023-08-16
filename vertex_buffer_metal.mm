#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#include "vertex_buffer.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::VertexBuffer::~VertexBuffer()
{
    for(auto& n : _data->_buffers)
    {
        if(n)
        {
            [(id<MTLBuffer>)n setPurgeableState:MTLPurgeableStateEmpty];
            [(id<MTLBuffer>)n release];
        }
    }
}

paz::VertexBuffer::VertexBuffer()
{
    _data = std::make_unique<Data>();
}

void paz::VertexBuffer::attribute(int dim, const std::vector<float>& data)
{
    check_size(dim, _numVertices, data);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data.data() length:
        sizeof(float)*dim*_numVertices options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<unsigned int>&
    data)
{
    check_size(dim, _numVertices, data);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data.data() length:
        sizeof(unsigned int)*dim*_numVertices options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const std::vector<int>& data)
{
    check_size(dim, _numVertices, data);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data.data() length:
        sizeof(int)*dim*_numVertices options:MTLStorageModeShared]);
}

#endif
