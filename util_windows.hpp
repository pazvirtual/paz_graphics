#ifndef PAZ_GRAPHICS_UTIL_D3D11_HPP
#define PAZ_GRAPHICS_UTIL_D3D11_HPP

#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include "PAZ_Graphics"
#include <d3d11.h>

namespace paz
{
    DXGI_FORMAT dxgi_format(int dim, paz::DataType type);
    std::string format_hresult(HRESULT hr);
}

#endif

#endif
