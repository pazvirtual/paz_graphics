#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "util_windows.hpp"
#include <sstream>
#include <iomanip>

#define CASE(a, b) case DataType::a: return DXGI_FORMAT_##b;

DXGI_FORMAT paz::dxgi_format(int dim, DataType type)
{
    if(dim == 1)
    {
        switch(type)
        {
            CASE(Float, R32_FLOAT);
            CASE(SInt, R32_SINT);
            CASE(UInt, R32_UINT);
            default: throw std::logic_error("Invalid data type.");
        }
    }
    else if(dim == 2)
    {
        switch(type)
        {
            CASE(Float, R32G32_FLOAT);
            CASE(SInt, R32G32_SINT);
            CASE(UInt, R32G32_UINT);
            default: throw std::logic_error("Invalid data type.");
        }
    }
    else if(dim == 4)
    {
        switch(type)
        {
            CASE(Float, R32G32B32A32_FLOAT);
            CASE(SInt, R32G32B32A32_SINT);
            CASE(UInt, R32G32B32A32_UINT);
            default: throw std::logic_error("Invalid data type.");
        }
    }
    throw std::runtime_error("Attribute dimensions must be 1, 2, or 4.");
}

std::string paz::format_hresult(HRESULT hr) noexcept
{
    std::ostringstream oss;
    oss << "HRESULT 0x" << std::hex << std::setfill('0') << std::setw(8) << hr;
    return oss.str();
}

std::wstring paz::utf8_to_wstring(const std::string& str) noexcept
{
    const int bufSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
        static_cast<int>(str.size()), nullptr, 0);
    std::wstring buf;
    buf.resize(bufSize);
    if(!MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.
        size()), const_cast<LPWSTR>(buf.data()), bufSize))
    {
        return L"UTF-8 TO WSTRING CONVERSION FAILED";
    }
    return buf;
}

#endif
