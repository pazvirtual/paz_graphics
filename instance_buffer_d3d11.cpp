#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

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
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

void paz::InstanceBuffer::addAttribute(int dim, const float* data, std::size_t
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
        throw std::runtime_error("Failed to create instance buffer (HRESULT " +
            std::to_string(hr) + ").");
    }
    const unsigned int slot = _data->_inputElemDescriptors.size();
    D3D11_INPUT_ELEMENT_DESC inputDescriptor = {};
    inputDescriptor.SemanticName = "ATTR";
    inputDescriptor.SemanticIndex = slot;
    inputDescriptor.Format = dxgi_format(dim, paz::
        DataType::Float);
    inputDescriptor.InputSlot = slot;
    inputDescriptor.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    inputDescriptor.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
    inputDescriptor.InstanceDataStepRate = 1;
    _data->_inputElemDescriptors.push_back(inputDescriptor);
    _data->_strides.push_back(dim*sizeof(float));
}

void paz::InstanceBuffer::addAttribute(int dim, const unsigned int* data, std::
    size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

void paz::InstanceBuffer::addAttribute(int dim, const int* data, std::size_t
    size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const float* data, std::
    size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const unsigned int*
    data, std::size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const int* data, std::
    size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
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
