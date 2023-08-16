#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

paz::InstanceBuffer::Data::~Data()
{
}

paz::InstanceBuffer::Data::Data()
{
}

paz::InstanceBuffer::InstanceBuffer()
{
}

paz::InstanceBuffer::InstanceBuffer(std::size_t size) : InstanceBuffer()
{
}

void paz::InstanceBuffer::Data::checkSize(int dim, std::size_t size)
{
}

void paz::InstanceBuffer::Data::addAttribute(int dim, DataType type)
{
}

void paz::InstanceBuffer::addAttribute(int dim, DataType type)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const GLfloat* data, std::size_t
    size)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const GLuint* data, std::size_t
    size)
{
}

void paz::InstanceBuffer::addAttribute(int dim, const GLint* data, std::size_t
    size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLfloat* data,
    std::size_t size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLuint* data,
    std::size_t size)
{
}

void paz::InstanceBuffer::subAttribute(std::size_t idx, const GLint* data, std::
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
