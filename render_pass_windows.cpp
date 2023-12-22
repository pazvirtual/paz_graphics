#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "util_windows.hpp"
#include "common.hpp"
#include "windows.hpp"

#define CHECK_PASS if(!CurPass) throw std::logic_error("No current render pass"\
    "."); else if(this != CurPass) throw std::logic_error("Render pass operati"\
    "ons cannot be interleaved.");

#define CASE(a, b) case paz::PrimitiveType::a: return \
    D3D11_PRIMITIVE_TOPOLOGY_##b;

static D3D11_PRIMITIVE_TOPOLOGY primitive_topology(paz::PrimitiveType t)
{
    switch(t)
    {
        CASE(Points, POINTLIST)
        CASE(Lines, LINELIST)
        CASE(LineStrip, LINESTRIP)
        CASE(Triangles, TRIANGLELIST)
        CASE(TriangleStrip, TRIANGLESTRIP)
        default: throw std::runtime_error("Invalid primitive type.");
    }
}

static constexpr float Clear[] = {0.f, 0.f, 0.f, 0.f};
static constexpr float Black[] = {0.f, 0.f, 0.f, 1.f};
static constexpr float White[] = {1.f, 1.f, 1.f, 1.f};

static const paz::RenderPass* CurPass;

paz::RenderPass::Data::~Data()
{
    if(_vertUniformBuf)
    {
        _vertUniformBuf->Release();
    }
    if(_fragUniformBuf)
    {
        _fragUniformBuf->Release();
    }
    if(_blendState)
    {
        _blendState->Release();
    }
}

void paz::RenderPass::Data::mapUniforms()
{
    if(_vertUniformBuf)
    {
        D3D11_MAPPED_SUBRESOURCE mappedSr;
        const auto hr = d3d_context()->Map(_vertUniformBuf, 0,
            D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
        if(hr)
        {
            throw std::runtime_error("Failed to map vertex function constant bu"
                "ffer (" + format_hresult(hr) + ").");
        }
        std::copy(_vertUniformData.begin(), _vertUniformData.end(),
            reinterpret_cast<unsigned char*>(mappedSr.pData));
        d3d_context()->Unmap(_vertUniformBuf, 0);
        d3d_context()->VSSetConstantBuffers(0, 1, &_vertUniformBuf);
    }
    if(_fragUniformBuf)
    {
        D3D11_MAPPED_SUBRESOURCE mappedSr;
        const auto hr = d3d_context()->Map(_fragUniformBuf, 0,
            D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
        if(hr)
        {
            throw std::runtime_error("Failed to map fragment function constant "
                "buffer (" + format_hresult(hr) + ").");
        }
        std::copy(_fragUniformData.begin(), _fragUniformData.end(),
            reinterpret_cast<unsigned char*>(mappedSr.pData));
        d3d_context()->Unmap(_fragUniformBuf, 0);
        d3d_context()->PSSetConstantBuffers(0, 1, &_fragUniformBuf);
    }
}

paz::RenderPass::RenderPass()
{
    initialize();
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const VertexFunction& vert,
    const FragmentFunction& frag, const std::vector<BlendMode>& modes)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_vert = vert._data;
    _data->_frag = frag._data;
    _data->_fbo = fbo._data;

    // Set blending mode. (Applies to all color render targets.)
    D3D11_BLEND_DESC blendDescriptor = {};
    blendDescriptor.IndependentBlendEnable = true;
    for(auto& n : blendDescriptor.RenderTarget)
    {
        n.BlendEnable = false;
        n.BlendOp = D3D11_BLEND_OP_ADD;
        n.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        n.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    for(std::size_t i = 0; i < modes.size(); ++i)
    {
        blendDescriptor.RenderTarget[i].BlendEnable = modes[i] != BlendMode::
            Disable;
        auto srcBlend = D3D11_BLEND_ONE;
        auto destBlend = D3D11_BLEND_ONE;
        if(modes[i] == BlendMode::One_InvSrcAlpha)
        {
            destBlend = D3D11_BLEND_INV_SRC_ALPHA;
        }
        else if(modes[i] == BlendMode::SrcAlpha_InvSrcAlpha)
        {
            srcBlend = D3D11_BLEND_SRC_ALPHA;
            destBlend = D3D11_BLEND_INV_SRC_ALPHA;
        }
        else if(modes[i] == BlendMode::InvSrcAlpha_SrcAlpha)
        {
            srcBlend = D3D11_BLEND_INV_SRC_ALPHA;
            destBlend = D3D11_BLEND_SRC_ALPHA;
        }
        else if(modes[i] == BlendMode::Zero_InvSrcAlpha)
        {
            srcBlend = D3D11_BLEND_ZERO;
            destBlend = D3D11_BLEND_INV_SRC_ALPHA;
        }
        else if(modes[i] != paz::BlendMode::One_One && modes[i] != BlendMode::
            Disable)
        {
            throw std::logic_error("Invalid blending function.");
        }
        blendDescriptor.RenderTarget[i].SrcBlend = srcBlend;
        blendDescriptor.RenderTarget[i].SrcBlendAlpha = srcBlend;
        blendDescriptor.RenderTarget[i].DestBlend = destBlend;
        blendDescriptor.RenderTarget[i].DestBlendAlpha = destBlend;
    }
    auto hr = d3d_device()->CreateBlendState(&blendDescriptor, &_data->
        _blendState);
    if(hr)
    {
        throw std::runtime_error("Failed to create blend state (" +
            format_hresult(hr) + ").");
    }

    // Check vertex and fragment function I/O compatibility. (Linking?)
    // ...

    // Get texture and sampler binding locations.
    ID3D11ShaderReflection* reflection;
    hr = D3DReflect(_data->_frag->_bytecode->GetBufferPointer(), _data->_frag->
        _bytecode->GetBufferSize(), IID_ID3D11ShaderReflection,
        reinterpret_cast<void**>(&reflection));
    if(hr)
    {
        throw std::runtime_error("Failed to get fragment function reflection ("
            + format_hresult(hr) + ").");
    }
    D3D11_SHADER_INPUT_BIND_DESC inputDescriptor;
    int i = 0;
    while(!reflection->GetResourceBindingDesc(i++, &inputDescriptor))
    {
        const std::string name = inputDescriptor.Name;
        if(name.size() > 7)
        {
            const auto ending = name.substr(name.size() - 7);
            if(ending == "Texture" || ending == "Sampler")
            {
                _data->_texAndSamplerSlots[name] = inputDescriptor.BindPoint;
            }
        }
    }

    // Create buffers to set uniforms.
    if(!_data->_vert->_uniforms.empty())
    {
        _data->_vertUniformData.resize(_data->_vert->_uniformBufSize);
        D3D11_BUFFER_DESC descriptor = {};
        descriptor.Usage = D3D11_USAGE_DYNAMIC;
        descriptor.ByteWidth = _data->_vert->_uniformBufSize;
        descriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        const auto hr = d3d_device()->CreateBuffer(&descriptor, nullptr,
            &_data->_vertUniformBuf);
        if(hr)
        {
            throw std::runtime_error("Failed to create constant buffer for vert"
                "ex function (" + format_hresult(hr) + ").");
        }
    }
    if(!_data->_frag->_uniforms.empty())
    {
        _data->_fragUniformData.resize(_data->_frag->_uniformBufSize);
        D3D11_BUFFER_DESC descriptor = {};
        descriptor.Usage = D3D11_USAGE_DYNAMIC;
        descriptor.ByteWidth = _data->_frag->_uniformBufSize;
        descriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        const auto hr = d3d_device()->CreateBuffer(&descriptor, nullptr,
            &_data->_fragUniformBuf);
        if(hr)
        {
            throw std::runtime_error("Failed to create constant buffer for frag"
                "ment function (" + format_hresult(hr) + ").");
        }
    }
}

paz::RenderPass::RenderPass(const VertexFunction& vert, const FragmentFunction&
    frag, const std::vector<BlendMode>& modes) : RenderPass(final_framebuffer(),
    vert, frag, modes) {}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    if(CurPass)
    {
        throw std::logic_error("Previous render pass was not ended.");
    }
    CurPass = this;

    if(!_data)
    {
        throw std::runtime_error("Render pass has not been initialized.");
    }

    d3d_context()->VSSetShader(_data->_vert->_shader, nullptr, 0);
    d3d_context()->PSSetShader(_data->_frag->_shader, nullptr, 0);

    begin_frame();
    _data->_rasterDescriptor.FillMode = D3D11_FILL_SOLID;
    _data->_rasterDescriptor.CullMode = D3D11_CULL_NONE;
    _data->_rasterDescriptor.FrontCounterClockwise = true;
    _data->_rasterDescriptor.DepthClipEnable = true;
    ID3D11RasterizerState* state;
    auto hr = d3d_device()->CreateRasterizerState(&_data->_rasterDescriptor,
        &state);
    if(hr)
    {
        throw std::runtime_error("Failed to create rasterizer state (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->RSSetState(state);
    state->Release();

    D3D11_DEPTH_STENCIL_DESC depthStencilDescriptor = {};
    ID3D11DepthStencilState* depthStencilState;
    hr = d3d_device()->CreateDepthStencilState(&depthStencilDescriptor,
        &depthStencilState);
    if(hr)
    {
        throw std::runtime_error("Failed to create depth/stencil state (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->OMSetDepthStencilState(depthStencilState, 1);
    depthStencilState->Release();

    d3d_context()->OMSetBlendState(_data->_blendState, nullptr, 0xffffffff);

    const auto numColor = _data->_fbo->_colorAttachments.size();
    std::vector<ID3D11RenderTargetView*> colorTargets(numColor);
    for(std::size_t i = 0; i < numColor; ++i)
    {
        colorTargets[i] = _data->_fbo->_colorAttachments[i]->_rtView;
    }
    d3d_context()->OMSetRenderTargets(numColor, numColor ? colorTargets.data() :
        nullptr, _data->_fbo->_depthStencilAttachment ? _data->_fbo->
        _depthStencilAttachment->_dsView : nullptr);

    for(std::size_t i = 0; i < std::min(colorLoadActions.size(), numColor); ++i)
    {
        if(colorLoadActions[i] == LoadAction::Clear)
        {
            d3d_context()->ClearRenderTargetView(colorTargets[i], Black);
        }
        else if(colorLoadActions[i] == LoadAction::FillZeros)
        {
            d3d_context()->ClearRenderTargetView(colorTargets[i], Clear);
        }
        else if(colorLoadActions[i] == LoadAction::FillOnes)
        {
            d3d_context()->ClearRenderTargetView(colorTargets[i], White);
        }
        else if(colorLoadActions[i] != LoadAction::DontCare && colorLoadActions[
            i] != LoadAction::Load)
        {
            throw std::runtime_error("Invalid color attachment load action.");
        }
    }
    if(_data->_fbo->_depthStencilAttachment)
    {
        if(depthLoadAction == LoadAction::Clear || depthLoadAction == LoadAction
            ::FillOnes)
        {
            d3d_context()->ClearDepthStencilView(_data->_fbo->
                _depthStencilAttachment->_dsView, D3D11_CLEAR_DEPTH, 1.f, 0);
        }
        else if(depthLoadAction == LoadAction::FillZeros)
        {
            d3d_context()->ClearDepthStencilView(_data->_fbo->
                _depthStencilAttachment->_dsView, D3D11_CLEAR_DEPTH, 0.f, 0);
        }
        else if(depthLoadAction != LoadAction::DontCare && depthLoadAction !=
            LoadAction::Load)
        {
            throw std::runtime_error("Invalid depth attachment load action.");
        }
    }

    D3D11_VIEWPORT viewport = {};
    if(_data->_fbo->_width)
    {
        viewport.Width = _data->_fbo->_width;
        viewport.Height = _data->_fbo->_height;
    }
    else
    {
        viewport.Width = _data->_fbo->_scale*Window::ViewportWidth();
        viewport.Height = _data->_fbo->_scale*Window::ViewportHeight();
    }
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;
    d3d_context()->RSSetViewports(1, &viewport);
}

void paz::RenderPass::depth(DepthTestMode mode)
{
    CHECK_PASS
    D3D11_DEPTH_STENCIL_DESC depthStencilDescriptor = {};
    depthStencilDescriptor.DepthEnable = mode != DepthTestMode::Disable;
    depthStencilDescriptor.DepthWriteMask = mode < DepthTestMode::Never ?
        D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;
    if(mode == DepthTestMode::NeverNoMask || mode == DepthTestMode::Never)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_NEVER;
    }
    else if(mode == DepthTestMode::LessNoMask || mode == DepthTestMode::Less)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_LESS;
    }
    else if(mode == DepthTestMode::EqualNoMask || mode == DepthTestMode::Equal)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_EQUAL;
    }
    else if(mode == DepthTestMode::LessEqualNoMask || mode == DepthTestMode::
        LessEqual)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    }
    else if(mode == DepthTestMode::GreaterNoMask || mode == DepthTestMode::
        Greater)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_GREATER;
    }
    else if(mode == DepthTestMode::NotEqualNoMask || mode == DepthTestMode::
        NotEqual)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
    }
    else if(mode == DepthTestMode::GreaterEqualNoMask || mode == DepthTestMode::
        GreaterEqual)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    }
    else if(mode == DepthTestMode::AlwaysNoMask || mode == DepthTestMode::
        Always)
    {
        depthStencilDescriptor.DepthFunc = D3D11_COMPARISON_ALWAYS;
    }
    ID3D11DepthStencilState* depthStencilState;
    const auto hr = d3d_device()->CreateDepthStencilState(
        &depthStencilDescriptor, &depthStencilState);
    if(hr)
    {
        throw std::runtime_error("Failed to create depth/stencil state (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->OMSetDepthStencilState(depthStencilState, 1);
    depthStencilState->Release();
}

void paz::RenderPass::end()
{
    CHECK_PASS
    d3d_context()->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    CurPass = nullptr;
}

void paz::RenderPass::cull(CullMode mode)
{
    CHECK_PASS
    D3D11_CULL_MODE newMode;
    switch(mode)
    {
        case CullMode::Disable: newMode = D3D11_CULL_NONE; break;
        case CullMode::Front: newMode = D3D11_CULL_FRONT; break;
        case CullMode::Back: newMode = D3D11_CULL_BACK; break;
        default: throw std::logic_error("Invalid culling mode.");
    }
    if(_data->_rasterDescriptor.CullMode != newMode)
    {
        _data->_rasterDescriptor.CullMode = newMode;
        ID3D11RasterizerState* state;
        auto hr = d3d_device()->CreateRasterizerState(&_data->_rasterDescriptor,
            &state);
        if(hr)
        {
            throw std::runtime_error("Failed to create rasterizer state (" +
                format_hresult(hr) + ").");
        }
        d3d_context()->RSSetState(state);
        state->Release();
    }
}

void paz::RenderPass::read(const std::string& name, const Texture& tex)
{
    CHECK_PASS
    if(_data->_texAndSamplerSlots.count(name + "Texture"))
    {
        d3d_context()->PSSetShaderResources(_data->_texAndSamplerSlots.at(name +
            "Texture"), 1, &tex._data->_resourceView);
        d3d_context()->PSSetSamplers(_data->_texAndSamplerSlots.at(name +
            "Sampler"), 1, &tex._data->_sampler);
    }
}

void paz::RenderPass::uniform(const std::string& name, int x)
{
    CHECK_PASS
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y)
{
    CHECK_PASS
    std::array<int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
{
    CHECK_PASS
    std::array<int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w)
{
    CHECK_PASS
    std::array<int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size)
{
    CHECK_PASS
    if(sizeof(int)*size > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vert->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_vertUniformData.begin() +
            std::get<0>(_data->_vert->_uniforms.at(name)));
    }
    if(_data->_frag->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_fragUniformData.begin() +
            std::get<0>(_data->_frag->_uniforms.at(name)));
    }
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x)
{
    CHECK_PASS
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y)
{
    CHECK_PASS
    std::array<unsigned int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z)
{
    CHECK_PASS
    std::array<unsigned int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w)
{
    CHECK_PASS
    std::array<unsigned int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size)
{
    CHECK_PASS
    if(sizeof(unsigned int)*size > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vert->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_vertUniformData.begin() +
            std::get<0>(_data->_vert->_uniforms.at(name)));
    }
    if(_data->_frag->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_fragUniformData.begin() +
            std::get<0>(_data->_frag->_uniforms.at(name)));
    }
}

void paz::RenderPass::uniform(const std::string& name, float x)
{
    CHECK_PASS
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y)
{
    CHECK_PASS
    std::array<float, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z)
{
    CHECK_PASS
    std::array<float, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w)
{
    CHECK_PASS
    std::array<float, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size)
{
    CHECK_PASS
    if(sizeof(float)*size > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vert->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_vertUniformData.begin() +
            std::get<0>(_data->_vert->_uniforms.at(name)));
    }
    if(_data->_frag->_uniforms.count(name))
    {
        std::copy(reinterpret_cast<const unsigned char*>(x), reinterpret_cast<
            const unsigned char*>(x + size), _data->_fragUniformData.begin() +
            std::get<0>(_data->_frag->_uniforms.at(name)));
    }
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices)
{
    CHECK_PASS
    if(!vertices._data->_numVertices)
    {
        return;
    }

    _data->mapUniforms();

    ID3D11InputLayout* layout;
    const auto hr = d3d_device()->CreateInputLayout(vertices._data->
        _inputElemDescriptors.data(), vertices._data->_inputElemDescriptors.
        size(), _data->_vert->_bytecode->GetBufferPointer(), _data->_vert->
        _bytecode->GetBufferSize(), &layout);
    if(hr)
    {
        throw std::runtime_error("Failed to create input layout (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->IASetInputLayout(layout);
    layout->Release();

    d3d_context()->IASetPrimitiveTopology(primitive_topology(type));
    const std::vector<unsigned int> offsets(vertices._data->_buffers.size(), 0);
    d3d_context()->IASetVertexBuffers(0, vertices._data->_buffers.size(),
        vertices._data->_buffers.data(), vertices._data->_strides.data(),
        offsets.data());
    d3d_context()->Draw(vertices._data->_numVertices, 0);
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices)
{
    CHECK_PASS
    if(!vertices._data->_numVertices || !indices._data->_numIndices)
    {
        return;
    }

    _data->mapUniforms();

    ID3D11InputLayout* layout;
    const auto hr = d3d_device()->CreateInputLayout(vertices._data->
        _inputElemDescriptors.data(), vertices._data->_inputElemDescriptors.
        size(), _data->_vert->_bytecode->GetBufferPointer(), _data->_vert->
        _bytecode->GetBufferSize(), &layout);
    if(hr)
    {
        throw std::runtime_error("Failed to create input layout (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->IASetInputLayout(layout);
    layout->Release();

    d3d_context()->IASetPrimitiveTopology(primitive_topology(type));
    const std::vector<unsigned int> offsets(vertices._data->_buffers.size(), 0);
    d3d_context()->IASetVertexBuffers(0, vertices._data->_buffers.size(),
        vertices._data->_buffers.data(), vertices._data->_strides.data(),
        offsets.data());
    d3d_context()->IASetIndexBuffer(indices._data->_buffer,
        DXGI_FORMAT_R32_UINT, 0);
    d3d_context()->DrawIndexed(indices._data->_numIndices, 0, 0);
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances)
{
    CHECK_PASS
    if(!vertices._data->_numVertices || !instances._data->_numInstances)
    {
        return;
    }

    _data->mapUniforms();

    ID3D11InputLayout* layout;
    auto inputElemDescriptors = vertices._data->_inputElemDescriptors;
    const auto startSlot = inputElemDescriptors.size();
    for(const auto& n : instances._data->_inputElemDescriptors)
    {
        inputElemDescriptors.push_back(n);
        inputElemDescriptors.back().InputSlot += startSlot;
    }
    const auto hr = d3d_device()->CreateInputLayout(inputElemDescriptors.data(),
        inputElemDescriptors.size(), _data->_vert->_bytecode->
        GetBufferPointer(), _data->_vert->_bytecode->GetBufferSize(), &layout);
    if(hr)
    {
        throw std::runtime_error("Failed to create input layout (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->IASetInputLayout(layout);
    layout->Release();

    d3d_context()->IASetPrimitiveTopology(primitive_topology(type));
    const std::vector<unsigned int> offsets(std::max(startSlot, instances.
        _data->_buffers.size()), 0);
    d3d_context()->IASetVertexBuffers(0, vertices._data->_buffers.size(),
        vertices._data->_buffers.data(), vertices._data->_strides.data(),
        offsets.data());
    d3d_context()->IASetVertexBuffers(startSlot, instances._data->_buffers.
        size(), instances._data->_buffers.data(), instances._data->_strides.
        data(), offsets.data());
    d3d_context()->DrawInstanced(vertices._data->_numVertices, instances._data->
        _numInstances, 0, 0);
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances, const IndexBuffer& indices)
{
    CHECK_PASS
    if(!vertices._data->_numVertices || !instances._data->_numInstances ||
        !indices._data->_numIndices)
    {
        return;
    }

    _data->mapUniforms();

    ID3D11InputLayout* layout;
    auto inputElemDescriptors = vertices._data->_inputElemDescriptors;
    const auto startSlot = inputElemDescriptors.size();
    for(const auto& n : instances._data->_inputElemDescriptors)
    {
        inputElemDescriptors.push_back(n);
        inputElemDescriptors.back().InputSlot += startSlot;
    }
    const auto hr = d3d_device()->CreateInputLayout(inputElemDescriptors.data(),
        inputElemDescriptors.size(), _data->_vert->_bytecode->
        GetBufferPointer(), _data->_vert->_bytecode->GetBufferSize(), &layout);
    if(hr)
    {
        throw std::runtime_error("Failed to create input layout (" +
            format_hresult(hr) + ").");
    }
    d3d_context()->IASetInputLayout(layout);
    layout->Release();

    d3d_context()->IASetPrimitiveTopology(primitive_topology(type));
    const std::vector<unsigned int> offsets(std::max(startSlot, instances.
        _data->_buffers.size()), 0);
    d3d_context()->IASetVertexBuffers(0, vertices._data->_buffers.size(),
        vertices._data->_buffers.data(), vertices._data->_strides.data(),
        offsets.data());
    d3d_context()->IASetVertexBuffers(startSlot, instances._data->_buffers.
        size(), instances._data->_buffers.data(), instances._data->_strides.
        data(), offsets.data());
    d3d_context()->IASetIndexBuffer(indices._data->_buffer,
        DXGI_FORMAT_R32_UINT, 0);
    d3d_context()->DrawIndexedInstanced(indices._data->_numIndices, instances.
        _data->_numInstances, 0, 0, 0);
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
    Framebuffer temp;
    temp._data = _data->_fbo;
    return temp;
}

#endif
