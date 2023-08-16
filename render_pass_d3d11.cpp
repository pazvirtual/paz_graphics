#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "util_d3d11.hpp"
#include "window.hpp"

#define CHECK_PASS if(!CurPass) throw std::logic_error("No current render pass"\
    "."); else if(this != CurPass) throw std::logic_error("Render pass operati"\
    "ons cannot be interleaved.");

static const paz::RenderPass* CurPass;

paz::RenderPass::RenderPass()
{
    initialize();
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const VertexFunction& vert,
    const FragmentFunction& frag, BlendMode mode)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_vert = vert._data;
    _data->_frag = frag._data;
    _data->_fbo = fbo._data;
}

paz::RenderPass::RenderPass(const VertexFunction& vert, const FragmentFunction&
    frag, BlendMode mode) : RenderPass(final_framebuffer(), vert, frag, mode) {}

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
        throw std::runtime_error("Failed to create rasterizer state (HRESULT " +
            std::to_string(hr) + ").");
    }
    d3d_context()->RSSetState(state);

    D3D11_DEPTH_STENCIL_DESC depthStencilDescriptor = {};
    ID3D11DepthStencilState * depthStencilState;
    hr = d3d_device()->CreateDepthStencilState(&depthStencilDescriptor,
        &depthStencilState);
    if(hr)
    {
        throw std::runtime_error("Failed to create depth/stencil state (HRESULT"
            " " + std::to_string(hr) + ").");
    }
    d3d_context()->OMSetDepthStencilState(depthStencilState, 1);

    const auto numColor = _data->_fbo->_colorAttachments.size();
    std::vector<ID3D11RenderTargetView*> colorTargets(numColor);
    for(std::size_t i = 0; i < numColor; ++i)
    {
        colorTargets[i] = _data->_fbo->_colorAttachments[i]->_rtView;
    }
    d3d_context()->OMSetRenderTargets(numColor, colorTargets.data(), _data->
        _fbo->_depthStencilAttachment->_dsView);

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
    viewport.MinDepth = -1.f; //TEMP - check other APIs
    viewport.MaxDepth = 1.f;
    d3d_context()->RSSetViewports(1, &viewport);
}

void paz::RenderPass::depth(DepthTestMode mode)
{
    CHECK_PASS
}

void paz::RenderPass::end()
{
    CHECK_PASS
    // ...
    CurPass = nullptr;
}

void paz::RenderPass::cull(CullMode mode)
{
    CHECK_PASS
}

void paz::RenderPass::read(const std::string& name, const Texture& tex)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices)
{
    CHECK_PASS

    ID3D11InputLayout* layout;
    const auto hr = d3d_device()->CreateInputLayout(vertices._data->
        _inputElemDescriptors.data(), vertices._data->_inputElemDescriptors.
        size(), _data->_vert->_bytecode->GetBufferPointer(), _data->_vert->
        _bytecode->GetBufferSize(), &layout);
    if(hr)
    {
        throw std::runtime_error("Failed to create input layout (HRESULT " +
            std::to_string(hr) + ").");
    }
    d3d_context()->IASetInputLayout(layout);
    d3d_context()->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); //TEMP
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
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances, const IndexBuffer& indices)
{
    CHECK_PASS
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
    Framebuffer temp;
    temp._data = _data->_fbo;
    return temp;
}

#endif
