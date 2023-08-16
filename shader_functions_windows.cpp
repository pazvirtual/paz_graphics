#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "PAZ_Graphics"
#include "util_windows.hpp"
#include "shading_lang.hpp"
#include "internal_data.hpp"
#include "common.hpp"
#include <d3dcompiler.h>

static constexpr int TypeSize = 4;

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
    if(in.empty())
    {
        return 0;
    }
    std::size_t offset = 0;
    for(const auto& n : in)
    {
        // Array elements are not packed.
        const auto arraySize = std::get<3>(n);
        const auto size = std::get<2>(n);
        if(arraySize > 1 && TypeSize*size != 16)
        {
            throw std::logic_error("NOT IMPLEMENTED");
        }

        // Constants must be 16 B-aligned.
        if(offset%16 && (offset + TypeSize*size - 1)/16 != offset/16)
        {
            offset += 16 - offset%16;
        }
        out[std::get<0>(n)] = {offset, std::get<1>(n), std::get<2>(n)};
        offset += TypeSize*size*arraySize;
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
        throw std::runtime_error("Failed to create vertex shader (" +
            format_hresult(hr) + ").");
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
        throw std::runtime_error("Failed to create fragment shader (" +
            format_hresult(hr) + ").");
    }
    _data->_uniformBufSize = process_uniforms(uniforms, _data->_uniforms);
}

#endif
