#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"

paz::IndexBuffer::Data::~Data()
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
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
}

paz::IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t size)
{
    throw std::logic_error(__FILE__ ":" + std::to_string(__LINE__) + ": NOT IMPLEMENTED");
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
