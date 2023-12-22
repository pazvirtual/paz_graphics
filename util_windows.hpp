#ifndef PAZ_GRAPHICS_UTIL_D3D11_HPP
#define PAZ_GRAPHICS_UTIL_D3D11_HPP

#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "windows.hpp"

namespace paz
{
    DXGI_FORMAT dxgi_format(int dim, paz::DataType type);
    std::string format_hresult(HRESULT hr);
}

#endif

#endif
