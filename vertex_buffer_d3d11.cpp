#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

static DXGI_FORMAT dxgi_format(int dim, paz::DataType type)
{
    if(type == paz::DataType::Float)
    {
        if(dim == 1)
        {
            return DXGI_FORMAT_R32_FLOAT;
        }
        if(dim == 2)
        {
            return DXGI_FORMAT_R32G32_FLOAT;
        }
        if(dim == 4)
        {
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }
    throw std::runtime_error("HERE");
}

paz::VertexBuffer::Data::~Data()
{
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

void paz::VertexBuffer::Data::checkSize(int dim, std::size_t size)
{
}

void paz::VertexBuffer::addAttribute(int dim, DataType type)
{
}

void paz::VertexBuffer::addAttribute(int dim, const float* data, std::size_t
    size)
{
    _data->checkSize(dim, size);
    _data->_buffers.emplace_back();
    D3D11_BUFFER_DESC bufDescriptor = {};
    bufDescriptor.Usage = D3D11_USAGE_DEFAULT;
    bufDescriptor.ByteWidth = sizeof(float)*size;
    bufDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA srData = {};
    srData.pSysMem = data;
    const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, &srData,
        &_data->_buffers.back());
    if(hr)
    {
        throw std::runtime_error("Failed to create vertex buffer (HRESULT " +
            std::to_string(hr) + ").");
    }
    const unsigned int slot = _data->_inputElemDescriptors.size();
    _data->_inputElemDescriptors.push_back({"ATTR", 0, dxgi_format(dim, paz::
        DataType::Float), slot, D3D11_APPEND_ALIGNED_ELEMENT,
        D3D11_INPUT_PER_VERTEX_DATA, 0});
    _data->_strides.push_back(dim*sizeof(float));
}

void paz::VertexBuffer::addAttribute(int dim, const unsigned int* data, std::
    size_t size)
{
}

void paz::VertexBuffer::addAttribute(int dim, const int* data, std::size_t size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const float* data, std::
    size_t size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const unsigned int* data,
    std::size_t size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const int* data, std::
    size_t size)
{
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
