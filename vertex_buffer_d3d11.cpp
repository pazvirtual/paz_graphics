#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

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
}

std::size_t paz::VertexBuffer::size() const
{
}

#endif
