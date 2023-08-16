#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>
#include <numeric>

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
    if(_lineLoopIndices)
    {
        [static_cast<id<MTLBuffer>>(_lineLoopIndices) setPurgeableState:
            MTLPurgeableStateEmpty];
        [static_cast<id<MTLBuffer>>(_lineLoopIndices) release];
    }
    if(_triangleFanIndices)
    {
        [static_cast<id<MTLBuffer>>(_triangleFanIndices) setPurgeableState:
            MTLPurgeableStateEmpty];
        [static_cast<id<MTLBuffer>>(_triangleFanIndices) release];
    }
}

void paz::VertexBuffer::Data::checkSize(int dim, std::size_t size)
{
    if(dim != 1 && dim != 2 && dim != 4)
    {
        throw std::runtime_error("Vertex attribute dimensions must be 1, 2, or "
            "4.");
    }
    const std::size_t m = size/dim;
    if(!_numVertices)
    {
        _numVertices = m;
        {
            std::vector<unsigned int> idx(_numVertices + 1);
            std::iota(idx.begin(), idx.end(), 0);
            idx.back() = 0;
            _lineLoopIndices = [DEVICE newBufferWithBytes:idx.data() length:
                sizeof(unsigned int)*idx.size() options:MTLStorageModeShared];
        }
        {
            std::vector<unsigned int> idx(_numVertices < 3 ? 0 : 3*_numVertices
                - 6);
            for(std::size_t i = 0; i < idx.size()/3; ++i)
            {
                idx[3*i] = 0;
                idx[3*i + 1] = i + 1;
                idx[3*i + 2] = i + 2;
            }
            _triangleFanIndices = [DEVICE newBufferWithBytes:idx.data() length:
                sizeof(unsigned int)*idx.size() options:MTLStorageModeShared];
        }
    }
    else if(m != _numVertices)
    {
        throw std::runtime_error("Number of vertices for each attribute must ma"
            "tch.");
    }
}

paz::VertexBuffer::VertexBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::VertexBuffer::attribute(int dim, const float* data, std::size_t size)
{
    _data->checkSize(dim, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        float)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const unsigned int* data, std::size_t
    size)
{
    _data->checkSize(dim, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        unsigned int)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::attribute(int dim, const int* data, std::size_t size)
{
    _data->checkSize(dim, size);

    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        int)*size options:MTLStorageModeShared]);
}

bool paz::VertexBuffer::empty() const
{
    return !_data->_numVertices;
}

#endif
