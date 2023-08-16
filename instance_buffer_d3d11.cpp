#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

paz::InstanceBuffer::Data::~Data()
{
}

paz::InstanceBuffer::InstanceBuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::InstanceBuffer::InstanceBuffer(std::size_t size) : InstanceBuffer()
{
    initialize();

    _data->_numInstances = size;
}

void paz::InstanceBuffer::Data::checkSize(int dim, std::size_t size)
{
}

void paz::InstanceBuffer::addAttribute(int dim, DataType type)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const float* data, std::size_t
    size)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const unsigned int* data, std::
    size_t size)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const int* data, std::size_t
    size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const float* data, std::
    size_t size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const unsigned int*
    data, std::size_t size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const int* data, std::
    size_t size)
{
}

bool paz::InstanceBuffer::empty() const
{
}

std::size_t paz::InstanceBuffer::size() const
{
}

#endif
