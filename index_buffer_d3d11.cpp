#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"

paz::IndexBuffer::Data::~Data()
{
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
}

void paz::IndexBuffer::sub(const unsigned int* data, std::size_t size)
{
}

bool paz::IndexBuffer::empty() const
{
}

std::size_t paz::IndexBuffer::size() const
{
}

#endif
