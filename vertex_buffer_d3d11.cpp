#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

paz::VertexBuffer::Data::~Data()
{
}

paz::VertexBuffer::Data::Data()
{
}

paz::VertexBuffer::VertexBuffer()
{
}

paz::VertexBuffer::VertexBuffer(std::size_t size) : VertexBuffer()
{
}

void paz::VertexBuffer::Data::checkSize(int dim, std::size_t size)
{
}

void paz::VertexBuffer::Data::addAttribute(int dim, DataType type)
{
}

void paz::VertexBuffer::addAttribute(int dim, DataType type)
{
}

void paz::VertexBuffer::addAttribute(int dim, const GLfloat* data, std::size_t
    size)
{
}

void paz::VertexBuffer::addAttribute(int dim, const GLuint* data, std::size_t
    size)
{
}

void paz::VertexBuffer::addAttribute(int dim, const GLint* data, std::size_t
    size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLfloat* data, std::
    size_t size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLuint* data, std::
    size_t size)
{
}

void paz::VertexBuffer::subAttribute(std::size_t idx, const GLint* data, std::
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
