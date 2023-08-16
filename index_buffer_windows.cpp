#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "common.hpp"
#include "util_windows.hpp"

paz::IndexBuffer::Data::~Data()
{
    if(_buffer)
    {
        _buffer->Release();
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
        D3D11_BUFFER_DESC bufDescriptor = {};
        bufDescriptor.Usage = D3D11_USAGE_DYNAMIC;
        bufDescriptor.ByteWidth = sizeof(unsigned int)*size;
        bufDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
        const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, nullptr,
            &_data->_buffer);
        if(hr)
        {
            throw std::runtime_error("Failed to create index buffer (" +
                format_hresult(hr) + ").");
        }
    }
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    if(size)
    {
        D3D11_BUFFER_DESC bufDescriptor = {};
        bufDescriptor.Usage = D3D11_USAGE_DEFAULT;
        bufDescriptor.ByteWidth = sizeof(unsigned int)*size;
        bufDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA srData = {};
        srData.pSysMem = data;
        const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, &srData,
            &_data->_buffer);
        if(hr)
        {
            throw std::runtime_error("Failed to create index buffer (" +
                format_hresult(hr) + ").");
        }
    }
}

void paz::IndexBuffer::sub(const unsigned int* data, std::size_t size)
{
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = d3d_context()->Map(_data->_buffer, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map index buffer (" +
            format_hresult(hr) + ").");
    }
    std::copy(data, data + size, reinterpret_cast<unsigned int*>(mappedSr.
        pData));
    d3d_context()->Unmap(_data->_buffer, 0);
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
