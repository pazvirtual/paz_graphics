#ifndef PAZ_GRAPHICS_UTIL_WINDOWS_HPP
#define PAZ_GRAPHICS_UTIL_WINDOWS_HPP

#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "windows.hpp"

namespace paz
{
    DXGI_FORMAT dxgi_format(int dim, paz::DataType type);
    std::string format_hresult(HRESULT hr) noexcept;
    std::wstring utf8_to_wstring(const std::string& str) noexcept;
}

#endif

#endif
