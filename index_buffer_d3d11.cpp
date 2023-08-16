#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"

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
// createbuffer dynamic(?)
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_numIndices = size;
    D3D11_BUFFER_DESC bufDescriptor = {};
    bufDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
    bufDescriptor.ByteWidth = sizeof(unsigned int)*size;
    bufDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA srData = {};
    srData.pSysMem = data;
    const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, &srData, &_data->
        _buffer);
    if(hr)
    {
        throw std::runtime_error("Failed to create index buffer (HRESULT " +
            std::to_string(hr) + ").");
    }
}

void paz::IndexBuffer::sub(const unsigned int* data, std::size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

bool paz::IndexBuffer::empty() const
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

std::size_t paz::IndexBuffer::size() const
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

#endif
