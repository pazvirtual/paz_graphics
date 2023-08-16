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
    //TEMP - purging before `paz::Window::EndFrame()` will break rendering
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

paz::VertexBuffer::VertexBuffer(std::size_t size) : VertexBuffer()
{
    _data->_numVertices = size;
}

void paz::VertexBuffer::addAttribute(int dim, DataType type)
{
    _data->_dims.push_back(dim);
    if(_data->_numVertices)
    {
        throw std::runtime_error("Vertex buffer size has not been set.");
    }
    std::size_t s = dim*_data->_numVertices;
    switch(type)
    {
        case DataType::SInt: s *= sizeof(int); break;
        case DataType::UInt: s *= sizeof(unsigned int); break;
        case DataType::Float: s *= sizeof(float); break;
        default: throw std::logic_error("Invalid data type.");
    }
    _data->_buffers.push_back([DEVICE newBufferWithLength:s options:
        MTLStorageModeShared]);
}

void paz::VertexBuffer::addAttribute(int dim, const float* data, std::size_t size)
{
    _data->_dims.push_back(dim);
    _data->checkSize(dim, size);
    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        float)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::addAttribute(int dim, const unsigned int* data, std::
    size_t size)
{
    _data->_dims.push_back(dim);
    _data->checkSize(dim, size);
    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        unsigned int)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::addAttribute(int dim, const int* data, std::size_t size)
{
    _data->_dims.push_back(dim);
    _data->checkSize(dim, size);
    _data->_buffers.push_back([DEVICE newBufferWithBytes:data length:sizeof(
        int)*size options:MTLStorageModeShared]);
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const float* data, std::
    size_t size)
{
    if(idx >= _data->_buffers.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    _data->checkSize(_data->_dims[idx], size);
    std::copy(data, data + size, reinterpret_cast<float*>([static_cast<id<
        MTLBuffer>>(_data->_buffers[idx]) contents]));
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const unsigned int* data,
    std::size_t size)
{
    if(idx >= _data->_buffers.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    _data->checkSize(_data->_dims[idx], size);
    std::copy(data, data + size, reinterpret_cast<unsigned int*>([static_cast<
        id<MTLBuffer>>(_data->_buffers[idx]) contents]));
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const int* data, std::
    size_t size)
{
    if(idx >= _data->_buffers.size())
    {
        throw std::logic_error("Attribute index " + std::to_string(idx) +
            " is out of range.");
    }
    _data->checkSize(_data->_dims[idx], size);
    std::copy(data, data + size, reinterpret_cast<int*>([static_cast<id<
        MTLBuffer>>(_data->_buffers[idx]) contents]));
}

bool paz::VertexBuffer::empty() const
{
    return !_data || !_data->_numVertices;
}

std::size_t paz::VertexBuffer::size() const
{
    return _data ? _data->_numVertices : 0;
}

#endif
