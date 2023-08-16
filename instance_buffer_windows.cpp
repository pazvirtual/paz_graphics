#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_windows.hpp"
#include "internal_data.hpp"
#include "common.hpp"

static constexpr int TypeSize = 4;

paz::InstanceBuffer::Data::~Data()
{
    for(auto& n : _buffers)
    {
        if(n)
        {
            n->Release();
        }
    }
}

void paz::InstanceBuffer::Data::addAttribute(int dim, DataType type, const void*
    data, std::size_t size)
{
    checkSize(dim, size);
    _buffers.emplace_back();
    _strides.push_back(TypeSize*dim);
    if(size)
    {
        D3D11_BUFFER_DESC bufDescriptor = {};
        bufDescriptor.Usage = D3D11_USAGE_DEFAULT;
        bufDescriptor.ByteWidth = TypeSize*size;
        bufDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA srData = {};
        srData.pSysMem = data;
        const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, &srData,
            &_buffers.back());
        if(hr)
        {
            throw std::runtime_error("Failed to create instance buffer (" +
                format_hresult(hr) + ").");
        }
    }
    const unsigned int slot = _inputElemDescriptors.size();
    D3D11_INPUT_ELEMENT_DESC inputDescriptor = {};
    inputDescriptor.SemanticName = "INST";
    inputDescriptor.SemanticIndex = slot;
    inputDescriptor.Format = dxgi_format(dim, type);
    inputDescriptor.InputSlot = slot;
    inputDescriptor.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    inputDescriptor.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
    inputDescriptor.InstanceDataStepRate = 1;
    _inputElemDescriptors.push_back(inputDescriptor);
}

paz::InstanceBuffer::InstanceBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::InstanceBuffer::InstanceBuffer(std::size_t size) : InstanceBuffer()
{
    _data->_numInstances = size;
}

void paz::InstanceBuffer::Data::checkSize(int dim, std::size_t size)
{
    if(dim != 1 && dim != 2 && dim != 4)
    {
        throw std::runtime_error("Instance attribute dimensions must be 1, 2, o"
            "r 4.");
    }
    const std::size_t m = size/dim;
    if(!_numInstances)
    {
        _numInstances = m;
    }
    else if(m != _numInstances)
    {
        throw std::runtime_error("Number of instances for each attribute must m"
            "atch.");
    }
}

void paz::InstanceBuffer::addAttribute(int dim, DataType type)
{
    if(!_data->_numInstances)
    {
        throw std::runtime_error("Instance buffer size has not been set.");
    }
    _data->_buffers.emplace_back();
    _data->_strides.push_back(TypeSize*dim);
    D3D11_BUFFER_DESC bufDescriptor = {};
    bufDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    bufDescriptor.ByteWidth = TypeSize*dim*_data->_numInstances;
    bufDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    const auto hr = d3d_device()->CreateBuffer(&bufDescriptor, nullptr, &_data->
        _buffers.back());
    if(hr)
    {
        throw std::runtime_error("Failed to create instance buffer (" +
            format_hresult(hr) + ").");
    }
    const unsigned int slot = _data->_inputElemDescriptors.size();
    D3D11_INPUT_ELEMENT_DESC inputDescriptor = {};
    inputDescriptor.SemanticName = "INST";
    inputDescriptor.SemanticIndex = slot;
    inputDescriptor.Format = dxgi_format(dim, type);
    inputDescriptor.InputSlot = slot;
    inputDescriptor.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    inputDescriptor.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
    inputDescriptor.InstanceDataStepRate = 1;
    _data->_inputElemDescriptors.push_back(inputDescriptor);
}

void paz::InstanceBuffer::addAttribute(int dim, const float* data, std::size_t
    size)
{
    _data->addAttribute(dim, DataType::Float, data, size);
}

void paz::InstanceBuffer::addAttribute(int dim, const unsigned int* data, std::
    size_t size)
{
    _data->addAttribute(dim, DataType::UInt, data, size);
}

void paz::InstanceBuffer::addAttribute(int dim, const int* data, std::size_t
    size)
{
    _data->addAttribute(dim, DataType::SInt, data, size);
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const float* data, std::
    size_t size)
{
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = d3d_context()->Map(_data->_buffers[idx], 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map instance buffer (" +
            format_hresult(hr) + ").");
    }
    std::copy(data, data + size, reinterpret_cast<float*>(mappedSr.pData));
    d3d_context()->Unmap(_data->_buffers[idx], 0);
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const unsigned int*
    data, std::size_t size)
{
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = d3d_context()->Map(_data->_buffers[idx], 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map instance buffer (" +
            format_hresult(hr) + ").");
    }
    std::copy(data, data + size, reinterpret_cast<unsigned int*>(mappedSr.
        pData));
    d3d_context()->Unmap(_data->_buffers[idx], 0);
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const int* data, std::
    size_t size)
{
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = d3d_context()->Map(_data->_buffers[idx], 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map instance buffer (" +
            format_hresult(hr) + ").");
    }
    std::copy(data, data + size, reinterpret_cast<int*>(mappedSr.pData));
    d3d_context()->Unmap(_data->_buffers[idx], 0);
}

bool paz::InstanceBuffer::empty() const
{
    return !_data || !_data->_numInstances;
}

std::size_t paz::InstanceBuffer::size() const
{
    return _data ? _data->_numInstances : 0;
}

#endif
