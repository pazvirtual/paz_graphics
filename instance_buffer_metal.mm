#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"

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

paz::InstanceBuffer::InstanceBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::InstanceBuffer::attribute(int dim, const float* data, std::size_t
    size)
{
    _data->checkSize(dim, size);

    _data->_buffers.emplace_back(reinterpret_cast<const unsigned char*>(data),
        reinterpret_cast<const unsigned char*>(data + size));
}

void paz::InstanceBuffer::attribute(int dim, const unsigned int* data, std::
    size_t size)
{
    _data->checkSize(dim, size);

    _data->_buffers.emplace_back(reinterpret_cast<const unsigned char*>(data),
        reinterpret_cast<const unsigned char*>(data + size));
}

void paz::InstanceBuffer::attribute(int dim, const int* data, std::size_t size)
{
    _data->checkSize(dim, size);

    _data->_buffers.emplace_back(reinterpret_cast<const unsigned char*>(data),
        reinterpret_cast<const unsigned char*>(data + size));
}

bool paz::InstanceBuffer::empty() const
{
    return !_data->_numInstances;
}

#endif
