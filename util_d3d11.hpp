#ifndef PAZ_GRAPHICS_UTIL_D3D11_HPP
#define PAZ_GRAPHICS_UTIL_D3D11_HPP

#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include <d3d11.h>

namespace paz
{
    DXGI_FORMAT dxgi_format(int dim, paz::DataType type);
}

#endif

#endif
