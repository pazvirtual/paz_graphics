#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static std::size_t type_size(paz::VertexBuffer::DataType t)
{
    if(t == paz::VertexBuffer::DataType::Float)
    {
        return sizeof(float);
    }
    else if(t == paz::VertexBuffer::DataType::SInt)
    {
        return sizeof(int);
    }
    else if(t == paz::VertexBuffer::DataType::UInt)
    {
        return sizeof(unsigned int);
    }

    throw std::logic_error("Invalid vertex attribute data type.");
}

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

paz::VertexBuffer::VertexBuffer(const std::vector<InputData>& data)
{
    _data = std::make_unique<Data>();

    _numVertices = (data.empty() ? 0 : data[0]._numVertices);
    for(const auto& n : data)
    {
        if(n._numVertices != _numVertices)
        {
            throw std::runtime_error("Number of vertices for each attribute mus"
                "t match.");
        }
        if(n._dim != 1 && n._dim != 2 && n._dim != 4)
        {
            throw std::runtime_error("Vertex attribute dimensions must be 1, 2,"
                " or 4.");
        }
        _data->_buffers.push_back([DEVICE newBufferWithBytes:n._data length:
            type_size(n._type)*n._dim*_numVertices options:
            MTLStorageModeShared]);
    }
}

#endif
