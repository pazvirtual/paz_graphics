#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "internal_data.hpp"
#include "common.hpp"
#include "util_macos.hh"
#import <MetalKit/MetalKit.h>

#define VIEW_CONTROLLER static_cast<ViewController*>([[static_cast<\
    AppDelegate*>([NSApp delegate]) window] contentViewController])
#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])
#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CHECK_PASS if(!_pass){ throw std::logic_error("No current render pass."\
    ); }else if(this != _pass){ throw std::logic_error("Render pass operations"\
    " cannot be interleaved."); }
#define CASE0(a, b) case paz::PrimitiveType::a: return MTLPrimitiveType##b;
#define CASE1(a, b, n) case MTLDataType##a: return {MTLVertexFormat##a, n* \
    sizeof(b)};

static_assert(sizeof(unsigned int) == 2 || sizeof(unsigned int) == 4, "Indices "
    "must be 16 or 32 bits.");
static constexpr MTLIndexType IndexType = (sizeof(unsigned int) == 2 ?
    MTLIndexTypeUInt16 : MTLIndexTypeUInt32);

static const paz::RenderPass* _pass;

static MTLPrimitiveType primitive_type(paz::PrimitiveType t)
{
    switch(t)
    {
        CASE0(Points, Point)
        CASE0(Lines, Line)
        CASE0(LineStrip, LineStrip)
        CASE0(Triangles, Triangle)
        CASE0(TriangleStrip, TriangleStrip)
        default: throw std::runtime_error("Invalid primitive type.");
    }
}

static std::pair<MTLVertexFormat, NSUInteger> vertex_format_stride(NSUInteger t)
{
    switch(t)
    {
        CASE1(Float, float, 1)
        CASE1(Float2, float, 2)
        CASE1(Float3, float, 3)
        CASE1(Float4, float, 4)
        CASE1(Int, int, 1)
        CASE1(Int2, int, 2)
        CASE1(Int3, int, 3)
        CASE1(Int4, int, 4)
        CASE1(UInt, unsigned int, 1)
        CASE1(UInt2, unsigned int, 2)
        CASE1(UInt3, unsigned int, 3)
        CASE1(UInt4, unsigned int, 4)
        default: throw std::logic_error("Invalid vertex data type.");
    }
}

static void check_attributes(const std::vector<void*>& a, const std::vector<
    std::size_t>& b, std::size_t n)
{
    if(a.size() != b.size())
    {
        throw std::invalid_argument("Vertex buffer has wrong number of attribut"
            "es for shader (got " + std::to_string(a.size()) + ", expected " +
            std::to_string(b.size()) + ").");
    }
    for(std::size_t i = 0; i < a.size(); ++i)
    {
        const std::size_t len = [static_cast<id<MTLBuffer>>(a[i]) length];
        const std::size_t expected = b[i]*n;
        if(len != expected)
        {
            throw std::invalid_argument("Vertex buffer attribute " + std::
                to_string(i) + " size mismatch (got " + std::to_string(len) +
                " bytes, expected " + std::to_string(expected) + " bytes).");
        }
    }
}

static id<MTLRenderPipelineState> create(const void* descriptor, std::
    unordered_map<std::string, int>& vertexArgs, std::unordered_map<std::string,
    int>& fragmentArgs, std::vector<std::size_t>& vertexAttributeStrides)
{
    // Get vertex attributes.
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor
        vertexDescriptor];
    int idx = 0;
    for(id n in [[static_cast<MTLRenderPipelineDescriptor*>(descriptor)
        vertexFunction] stageInputAttributes])
    {
        const auto formatStride = vertex_format_stride([n attributeType]);
        [vertexDescriptor attributes][idx].format = formatStride.first;
        [vertexDescriptor attributes][idx].bufferIndex = idx;
        [vertexDescriptor attributes][idx].offset = 0;
        [vertexDescriptor layouts][idx].stride = formatStride.second;
        vertexAttributeStrides.push_back(formatStride.second);
        ++idx;
    }
    [static_cast<MTLRenderPipelineDescriptor*>(descriptor) setVertexDescriptor:
        vertexDescriptor];

    // Get uniforms.
    MTLRenderPipelineReflection* reflection;
    NSError* error = nil;
    id<MTLRenderPipelineState> pipelineState = [DEVICE
        newRenderPipelineStateWithDescriptor:static_cast<
        MTLRenderPipelineDescriptor*>(descriptor) options:
        MTLPipelineOptionArgumentInfo reflection:&reflection error:&error];
    if(!pipelineState)
    {
        throw std::runtime_error([[NSString stringWithFormat:@"Failed to create"
            " pipeline state: %@", [error localizedDescription]] UTF8String]);
    }
    for(id n in [reflection vertexArguments])
    {
        vertexArgs[[[n name] UTF8String]] = [n index];
    }
    for(id n in [reflection fragmentArguments])
    {
        fragmentArgs[[[n name] UTF8String]] = [n index];
    }
    return pipelineState;
}

paz::RenderPass::Data::~Data()
{
    if(_pipelineState)
    {
        [static_cast<id<MTLRenderPipelineState>>(_pipelineState) release];
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
    MTLRenderPipelineDescriptor* pipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineDescriptor setVertexFunction:static_cast<id<MTLFunction>>(_data->
        _vert->_function)];
    [pipelineDescriptor setFragmentFunction:static_cast<id<MTLFunction>>(_data->
        _frag->_function)];
    for(std::size_t i = 0; i < _data->_fbo->_colorAttachments.size(); ++i)
    {
        [[pipelineDescriptor colorAttachments][i] setPixelFormat:
            [static_cast<id<MTLTexture>>(_data->_fbo->_colorAttachments[i]->
            _texture) pixelFormat]];
        if(i < modes.size() && modes[i] != paz::BlendMode::Disable)
        {
            [[pipelineDescriptor colorAttachments][i] setBlendingEnabled:YES];
            [[pipelineDescriptor colorAttachments][i] setRgbBlendOperation:
                MTLBlendOperationAdd];
            [[pipelineDescriptor colorAttachments][i] setAlphaBlendOperation:
                MTLBlendOperationAdd];
            if(modes[i] == paz::BlendMode::One_One)
            {
                [[pipelineDescriptor colorAttachments][i]
                    setSourceRGBBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][i]
                    setSourceAlphaBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationRGBBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationAlphaBlendFactor:MTLBlendFactorOne];
            }
            else if(modes[i] == paz::BlendMode::One_InvSrcAlpha)
            {
                [[pipelineDescriptor colorAttachments][i]
                    setSourceRGBBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][i]
                    setSourceAlphaBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationRGBBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationAlphaBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
            }
            else if(modes[i] == paz::BlendMode::SrcAlpha_InvSrcAlpha)
            {
                [[pipelineDescriptor colorAttachments][i]
                    setSourceRGBBlendFactor:MTLBlendFactorSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setSourceAlphaBlendFactor:MTLBlendFactorSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationRGBBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationAlphaBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
            }
            else if(modes[i] == paz::BlendMode::InvSrcAlpha_SrcAlpha)
            {
                [[pipelineDescriptor colorAttachments][i]
                    setSourceRGBBlendFactor:MTLBlendFactorOneMinusSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setSourceAlphaBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationRGBBlendFactor:MTLBlendFactorSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationAlphaBlendFactor:MTLBlendFactorSourceAlpha];
            }
            else if(modes[i] == paz::BlendMode::Zero_InvSrcAlpha)
            {
                [[pipelineDescriptor colorAttachments][i]
                    setSourceRGBBlendFactor:MTLBlendFactorZero];
                [[pipelineDescriptor colorAttachments][i]
                    setSourceAlphaBlendFactor:MTLBlendFactorZero];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationRGBBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
                [[pipelineDescriptor colorAttachments][i]
                    setDestinationAlphaBlendFactor:
                    MTLBlendFactorOneMinusSourceAlpha];
            }
            else
            {
                throw std::runtime_error("Invalid blending function.");
            }
        }
    }
    if(_data->_fbo->_depthStencilAttachment)
    {
        [pipelineDescriptor setDepthAttachmentPixelFormat:[static_cast<id<
            MTLTexture>>(_data->_fbo->_depthStencilAttachment->_texture)
            pixelFormat]];
    }
    _data->_pipelineState = create(pipelineDescriptor, _data->_vertexArgs,
        _data->_fragmentArgs, _data->_vertexAttributeStrides);
    [pipelineDescriptor release];
}

paz::RenderPass::RenderPass(const VertexFunction& vert, const FragmentFunction&
    frag, const std::vector<BlendMode>& modes) : RenderPass(final_framebuffer(),
    vert, frag, modes) {}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    if(_pass)
    {
        throw std::logic_error("Previous render pass was not ended.");
    }
    _pass = this;

    if(!_data)
    {
        throw std::runtime_error("Render pass has not been initialized.");
    }

    [RENDERER ensureCommandBuffer];

    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor
        alloc] init];
    if(!renderPassDescriptor)
    {
        throw std::runtime_error("Failed to create render pass descriptor.");
    }

    for(std::size_t i = 0; i < _data->_fbo->_colorAttachments.size(); ++i)
    {
        [[renderPassDescriptor colorAttachments][i] setTexture:static_cast<id<
            MTLTexture>>(_data->_fbo->_colorAttachments[i]->_texture)];
        if(colorLoadActions.empty() || colorLoadActions[i] == LoadAction::
            DontCare)
        {
            [[renderPassDescriptor colorAttachments][i] setLoadAction:
                MTLLoadActionDontCare];
        }
        else if(colorLoadActions[i] == LoadAction::Clear || colorLoadActions[i]
            == LoadAction::FillOnes || colorLoadActions[i] == LoadAction::
            FillZeros)
        {
            if(colorLoadActions[i] == LoadAction::FillOnes)
            {
                [[renderPassDescriptor colorAttachments][i] setClearColor:
                    MTLClearColorMake(1., 1., 1., 1.)];
            }
            else if(colorLoadActions[i] == LoadAction::FillZeros)
            {
                [[renderPassDescriptor colorAttachments][i] setClearColor:
                    MTLClearColorMake(0., 0., 0., 0.)];
            }
            [[renderPassDescriptor colorAttachments][i] setLoadAction:
                MTLLoadActionClear];
        }
        else if(colorLoadActions[i] == LoadAction::Load)
        {
            [[renderPassDescriptor colorAttachments][i] setLoadAction:
                MTLLoadActionLoad];
        }
        else
        {
            throw std::runtime_error("Invalid color attachment load action.");
        }
        [[renderPassDescriptor colorAttachments][i] setStoreAction:
            MTLStoreActionStore];
    }

    if(_data->_fbo->_depthStencilAttachment)
    {
        [[renderPassDescriptor depthAttachment] setTexture:static_cast<id<
            MTLTexture>>(_data->_fbo->_depthStencilAttachment->_texture)];
        if(depthLoadAction == LoadAction::DontCare)
        {
            [[renderPassDescriptor depthAttachment] setLoadAction:
                MTLLoadActionDontCare];
        }
        else if(depthLoadAction == LoadAction::Clear || depthLoadAction ==
            LoadAction::FillOnes || depthLoadAction == LoadAction::FillZeros)
        {
            if(depthLoadAction == LoadAction::FillZeros)
            {
                [[renderPassDescriptor depthAttachment] setClearDepth:0.];
            }
            [[renderPassDescriptor depthAttachment] setLoadAction:
                MTLLoadActionClear];
        }
        else if(depthLoadAction == LoadAction::Load)
        {
            [[renderPassDescriptor depthAttachment] setLoadAction:
                MTLLoadActionLoad];
        }
        else
        {
            throw std::runtime_error("Invalid depth attachment load action.");
        }
        [[renderPassDescriptor depthAttachment] setStoreAction:
            MTLStoreActionStore];
    }

    _data->_renderEncoder = [[RENDERER commandBuffer]
        renderCommandEncoderWithDescriptor:renderPassDescriptor];

    [renderPassDescriptor release];

    if(_data->_fbo->_width)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setViewport:{0., 0., static_cast<double>(_data->_fbo->_width),
            static_cast<double>(_data->_fbo->_height), 0., 1.}];
    }

    // Flip winding order to CCW to match OpenGL standard.
    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        setFrontFacingWinding:MTLWindingCounterClockwise];

    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        setRenderPipelineState:static_cast<id<MTLRenderPipelineState>>(_data->
        _pipelineState)];
}

void paz::RenderPass::depth(DepthTestMode mode)
{
    CHECK_PASS
    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        setDepthStencilState:depth_stencil_state(mode)];
}

void paz::RenderPass::end()
{
    CHECK_PASS
    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        endEncoding];
    _data->_renderEncoder = nullptr;
    for(auto n : _data->_fbo->_colorAttachments)
    {
        n->ensureMipmaps();
    }
    if(_data->_fbo->_depthStencilAttachment)
    {
        _data->_fbo->_depthStencilAttachment->ensureMipmaps();
    }
    _pass = nullptr;
}

void paz::RenderPass::cull(CullMode mode)
{
    CHECK_PASS
    if(mode == CullMode::Disable)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setCullMode:MTLCullModeNone];
    }
    else if(mode == CullMode::Front)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setCullMode:MTLCullModeFront];
    }
    else if(mode == CullMode::Back)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setCullMode:MTLCullModeBack];
    }
    else
    {
        throw std::runtime_error("Invalid culling mode.");
    }
}

void paz::RenderPass::read(const std::string& name, const Texture& tex)
{
    CHECK_PASS
    const std::string textureName = name + "Texture";
    if(_data->_vertexArgs.count(textureName))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexTexture:static_cast<id<MTLTexture>>(tex._data->_texture)
            atIndex:_data->_vertexArgs.at(textureName)];
        const std::string samplerName = name + "Sampler";
        if(!_data->_vertexArgs.count(samplerName))
        {
            throw std::logic_error("Vertex function takes texture as argument b"
                "ut not its sampler.");
        }
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexSamplerState:static_cast<id<MTLSamplerState>>(tex._data->
            _sampler) atIndex:_data->_vertexArgs.at(samplerName)];
    }
    if(_data->_fragmentArgs.count(textureName))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentTexture:static_cast<id<MTLTexture>>(tex._data->_texture)
            atIndex:_data->_fragmentArgs.at(textureName)];
        const std::string samplerName = name + "Sampler";
        if(!_data->_fragmentArgs.count(samplerName))
        {
            throw std::logic_error("Fragment function takes texture as argument"
                " but not its sampler.");
        }

        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentSamplerState:static_cast<id<MTLSamplerState>>(tex._data->
            _sampler) atIndex:_data->_fragmentArgs.at(samplerName)];
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
    const auto l = sizeof(int)*size;
    if(l > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:l atIndex:_data->_vertexArgs.at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:l atIndex:_data->_fragmentArgs.at(name)];
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
    const auto l = sizeof(unsigned int)*size;
    if(l > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:l atIndex:_data->_vertexArgs.at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:l atIndex:_data->_fragmentArgs.at(name)];
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
    const auto l = sizeof(float)*size;
    if(l > 4*1024) //TEMP - `set*Bytes` limitation
    {
        throw std::runtime_error("Too many bytes to send without buffer.");
    }
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:l atIndex:_data->_vertexArgs.at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:l atIndex:_data->_fragmentArgs.at(name)];
    }
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices)
{
    CHECK_PASS
    check_attributes(vertices._data->_buffers, _data->_vertexAttributeStrides,
        vertices._data->_numVertices);
    if(!vertices._data->_numVertices)
    {
        return;
    }

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        drawPrimitives:primitive_type(type) vertexStart:0 vertexCount:vertices.
        _data->_numVertices];
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices)
{
    CHECK_PASS
    check_attributes(vertices._data->_buffers, _data->_vertexAttributeStrides,
        vertices._data->_numVertices);
    if(!vertices._data->_numVertices || !indices._data->_numIndices)
    {
        return;
    }

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        drawIndexedPrimitives:primitive_type(type) indexCount:indices._data->
        _numIndices indexType:IndexType indexBuffer:static_cast<id<MTLBuffer>>(
        indices._data->_data) indexBufferOffset:0];
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances)
{
    CHECK_PASS
//    check_attributes(vertices._data->_buffers, instances._data->_buffers,
//        _data->_vertexAttributeStrides, vertices._data->_numVertices,
//        instances._data->_numVertices);
    if(!vertices._data->_numVertices || !instances._data->_numInstances)
    {
        return;
    }

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    for(std::size_t i = 0; i < instances._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(instances._data->
            _buffers[i]) offset:0 atIndex:vertices._data->_buffers.size() + i];
    }

    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        drawPrimitives:primitive_type(type) vertexStart:0 vertexCount:vertices.
        _data->_numVertices instanceCount:instances._data->_numInstances];
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances, const IndexBuffer& indices)
{
    CHECK_PASS
//    check_attributes(vertices._data->_buffers, instances._data->_buffers,
//        _data->_vertexAttributeStrides, vertices._data->_numVertices,
//        instances._data->_numVertices);
    if(!vertices._data->_numVertices || !instances._data->_numInstances ||
        !indices._data->_numIndices)
    {
        return;
    }

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    for(std::size_t i = 0; i < instances._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(instances._data->
            _buffers[i]) offset:0 atIndex:vertices._data->_buffers.size() + i];
    }

    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        drawIndexedPrimitives:primitive_type(type) indexCount:indices._data->
        _numIndices indexType:IndexType indexBuffer:static_cast<id<MTLBuffer>>(
        indices._data->_data) indexBufferOffset:0 instanceCount:instances.
        _data->_numInstances];
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
    Framebuffer temp;
    temp._data = _data->_fbo;
    return temp;
}

#endif
