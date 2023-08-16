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

paz::IndexBuffer::IndexBuffer() {}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    _data->_data = [DEVICE newBufferWithBytes:data length:sizeof(unsigned int)*
        size options:MTLStorageModeShared];
    {
        std::vector<unsigned int> idx(size + 1);
        std::copy(data, data + size, idx.begin());
        idx.back() = idx[0];
        _data->_lineLoopIndices = [DEVICE newBufferWithBytes:idx.data() length:
            sizeof(unsigned int)*idx.size() options:MTLStorageModeShared];
    }
    {
        std::vector<unsigned int> idx(size < 3 ? 0 : 3*size - 6);
        for(std::size_t i = 0; i < idx.size()/3; ++i)
        {
            idx[3*i] = data[0];
            idx[3*i + 1] = data[i + 1];
            idx[3*i + 2] = data[i + 2];
        }
        _data->_triangleFanIndices = [DEVICE newBufferWithBytes:idx.data()
            length:sizeof(unsigned int)*idx.size() options:
            MTLStorageModeShared];
    }
}

bool paz::IndexBuffer::empty() const
{
    return !_data->_numIndices;
}

#endif
