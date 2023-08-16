#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#include "common.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

paz::IndexBuffer::Data::~Data()
{
    //TEMP - purging before `paz::Window::EndFrame()` will break rendering
    if(_data)
    {
        [static_cast<id<MTLBuffer>>(_data) setPurgeableState:
            MTLPurgeableStateEmpty];
        [static_cast<id<MTLBuffer>>(_data) release];
    }
}

paz::IndexBuffer::IndexBuffer()
{
    initialize();
}

paz::IndexBuffer::IndexBuffer(std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    if(size)
    {
        _data->_data = [DEVICE newBufferWithLength:sizeof(unsigned int)*size
            options:MTLStorageModeShared];
    }
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    if(size)
    {
        _data->_data = [DEVICE newBufferWithBytes:data length:sizeof(unsigned
        int)*size options:MTLStorageModeShared];
    }
}

void paz::IndexBuffer::sub(const unsigned int* data, std::size_t size)
{
    if(size != _data->_numIndices)
    {
        throw std::runtime_error("Number of instances is fixed.");
    }
    std::copy(data, data + size, reinterpret_cast<unsigned int*>([static_cast<
        id<MTLBuffer>>(_data->_data) contents]));
}

bool paz::IndexBuffer::empty() const
{
    return !_data || !_data->_numIndices;
}

std::size_t paz::IndexBuffer::size() const
{
    return _data ? _data->_numIndices : 0;
}

#endif
