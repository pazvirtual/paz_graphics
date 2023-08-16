#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "shading_lang.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include <d3dcompiler.h>

paz::VertexFunction::Data::~Data()
{
    if(_shader)
    {
        _shader->Release();
    }
}

paz::FragmentFunction::Data::~Data()
{
    if(_shader)
    {
        _shader->Release();
    }
}

paz::VertexFunction::VertexFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::FragmentFunction::FragmentFunction()
{
    initialize();

    _data = std::make_shared<Data>();
}

static std::size_t process_uniforms(const std::vector<std::tuple<std::string,
    paz::DataType, int, int>>& in, std::unordered_map<std::string, std::tuple<
    std::size_t, paz::DataType, int>>& out)
{
    // Note that all basic data types are 32 b.
    if(in.empty())
    {
        return 0;
    }
    std::size_t offset = 0;
    for(const auto& n : in)
    {
        // Array elements are not packed.
        if(std::get<3>(n) > 1)
        {
            throw std::logic_error("NOT IMPLEMENTED");
        }

        // Constants must be 16 B-aligned.
        const auto size = std::get<2>(n);
        if(offset%16 && (offset + 4*size - 1)/16 != offset/16)
        {
            offset += 16 - offset%16;
        }
        out[std::get<0>(n)] = {offset, std::get<1>(n), std::get<2>(n)};
        offset += 4*size;
    }
    return ((offset + 15)/16)*16;
}

paz::VertexFunction::VertexFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
    std::vector<std::tuple<std::string, DataType, int, int>> uniforms;
    const std::string hlsl = vert2hlsl(src, uniforms);
    ID3DBlob* error;
    auto hr = D3DCompile(hlsl.c_str(), hlsl.size(), nullptr, nullptr, nullptr,
        "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &_data->_bytecode,
        &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile vertex shader: " + std::
            string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    hr = d3d_device()->CreateVertexShader(_data->_bytecode->GetBufferPointer(),
        _data->_bytecode->GetBufferSize(), nullptr, &_data->_shader);
    if(hr)
    {
        throw std::runtime_error("Failed to create vertex shader (HRESULT " +
            std::to_string(hr) + ").");
    }
    _data->_uniformBufSize = process_uniforms(uniforms, _data->_uniforms);
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
    std::vector<std::tuple<std::string, DataType, int, int>> uniforms;
    const std::string hlsl = frag2hlsl(src, uniforms);
    ID3DBlob* error;
    auto hr = D3DCompile(hlsl.c_str(), hlsl.size(), nullptr, nullptr, nullptr,
        "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &_data->_bytecode,
        &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile fragment shader: " + std::
            string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    hr = d3d_device()->CreatePixelShader(_data->_bytecode->GetBufferPointer(),
        _data->_bytecode->GetBufferSize(), nullptr, &_data->_shader);
    if(hr)
    {
        throw std::runtime_error("Failed to create fragment shader (HRESULT " +
            std::to_string(hr) + ").");
    }
    _data->_uniformBufSize = process_uniforms(uniforms, _data->_uniforms);
}

#endif
