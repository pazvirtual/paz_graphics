#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"

DXGI_FORMAT paz::dxgi_format(int dim, DataType type)
{
    if(type == DataType::Float)
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
    throw std::runtime_error("INCOMPLETE");
}

#endif
