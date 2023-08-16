#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.hh"
#import "view_controller.hh"
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
    for(auto& n : _buffers)
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
#if 0
    _numVertices = (data.empty() ? 0 : data[0]._numVertices);
    const std::size_t numAttribs = data.size();
    std::vector<std::size_t> attribSizes(numAttribs);
    std::size_t vertexSize = 0;
    for(std::size_t i = 0; i < numAttribs; ++i)
    {
        attribSizes[i] = type_size(data[i]._type)*data[i]._dim;
        vertexSize += attribSizes[i];
    }
    const std::size_t length = vertexSize*_numVertices;
    std::vector<unsigned char> bytes(length);
    for(std::size_t v = 0; v < _numVertices; ++v) // vertex
    {
        std::size_t offset = 0;
        for(std::size_t a = 0; a < numAttribs; ++a) // attribute
        {
            for(std::size_t b = 0; b < attribSizes[a]; ++b) // byte
            {
                const std::size_t i = vertexSize*v + offset + b;
                const std::size_t j = attribSizes[a]*v + b;
                bytes[i] = *(reinterpret_cast<const unsigned char*>(data[a].
                    _data) + j);
            }
            offset += attribSizes[a];
        }
    }
    _data = [DEVICE newBufferWithBytes:bytes.data() length:length options:
        MTLStorageModeShared];
#else
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
        _buffers.push_back([DEVICE newBufferWithBytes:n._data length:type_size(
            n._type)*n._dim*_numVertices options:MTLStorageModeShared]);
    }
#endif
}

#endif
