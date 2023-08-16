#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
//#include "opengl2d3d.hpp"
#include "internal_data.hpp"
#include "window.hpp"
#include <d3dcompiler.h>

//TEMP
static const std::string VSrc = 1 + R"===(
struct InputData
{
    float4 pos : ATTR0;
};
struct OutputData
{
    float4 glPosition : SV_Position;
};
OutputData main(InputData input)
{
    OutputData output;
    output.glPosition = input.pos;
    return output;
}
)===";
static const std::string FSrc = 1 + R"===(
struct InputData{ float4 glPosition : SV_Position; }; //TEMP
struct OutputData
{
    float4 color : SV_TARGET0;
};
OutputData main(InputData input)
{
    OutputData output;
    output.color = float4(0.5, 0.5, 1, 1);
    return output;
}
)===";
//TEMP

paz::VertexFunction::Data::~Data()
{
}

paz::FragmentFunction::Data::~Data()
{
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

paz::VertexFunction::VertexFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
    ID3DBlob* error;
    auto hr = D3DCompile(VSrc.c_str(), VSrc.size(), nullptr, nullptr, nullptr,
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
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();
    ID3DBlob* bytecode;
    ID3DBlob* error;
    auto hr = D3DCompile(FSrc.c_str(), FSrc.size(), nullptr, nullptr, nullptr,
        "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile fragment shader: " + std::
            string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    hr = d3d_device()->CreatePixelShader(bytecode->GetBufferPointer(),
        bytecode->GetBufferSize(), nullptr, &_data->_shader);
    if(hr)
    {
        throw std::runtime_error("Failed to create fragment shader (HRESULT " +
            std::to_string(hr) + ").");
    }
}

#endif
