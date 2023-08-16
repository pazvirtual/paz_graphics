#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "internal_data.hpp"
#include "window.hpp"
#include "util_metal.hh"
#import <MetalKit/MetalKit.h>

#define VIEW_CONTROLLER static_cast<ViewController*>([[static_cast<\
    AppDelegate*>([NSApp delegate]) window] contentViewController])
#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])
#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CASE(a, b, n) case MTLDataType##a: return {MTLVertexFormat##a, n* \
    sizeof(b)};

static_assert(sizeof(unsigned int) == 2 || sizeof(unsigned int) == 4, "Indices "
    "must be 16 or 32 bits.");
static constexpr MTLIndexType IndexType = (sizeof(unsigned int) == 2 ?
    MTLIndexTypeUInt16 : MTLIndexTypeUInt32);

static std::pair<MTLVertexFormat, NSUInteger> vertex_format_stride(NSUInteger t)
{
    switch(t)
    {
        CASE(Float, float, 1)
        CASE(Float2, float, 2)
        CASE(Float3, float, 3)
        CASE(Float4, float, 4)
        CASE(Int, int, 1)
        CASE(Int2, int, 2)
        CASE(Int3, int, 3)
        CASE(Int4, int, 4)
        CASE(UInt, unsigned int, 1)
        CASE(UInt2, unsigned int, 2)
        CASE(UInt3, unsigned int, 3)
        CASE(UInt4, unsigned int, 4)
        default: throw std::logic_error("Invalid vertex data type.");
    }
}

static void check_attributes(const std::vector<void*>& a, std::vector<std::
    size_t>& b, std::size_t n)
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

paz::RenderPass::RenderPass() {}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const VertexFunction& vert,
    const FragmentFunction& frag, BlendMode blendMode)
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
        if(blendMode != paz::BlendMode::Disable)
        {
            [[pipelineDescriptor colorAttachments][i] setBlendingEnabled:YES];
            [[pipelineDescriptor colorAttachments][i] setRgbBlendOperation:
                MTLBlendOperationAdd];
            [[pipelineDescriptor colorAttachments][i] setAlphaBlendOperation:
                MTLBlendOperationAdd];
            if(blendMode == paz::BlendMode::Additive)
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
            else if(blendMode == paz::BlendMode::Blend)
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
            // ...
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
    frag, BlendMode blendMode)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_vert = vert._data;
    _data->_frag = frag._data;
    MTLRenderPipelineDescriptor* pipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineDescriptor setVertexFunction:static_cast<id<MTLFunction>>(_data->
        _vert->_function)];
    [pipelineDescriptor setFragmentFunction:static_cast<id<MTLFunction>>(_data->
        _frag->_function)];
    [[pipelineDescriptor colorAttachments][0] setPixelFormat:[[VIEW_CONTROLLER
        mtkView] colorPixelFormat]];
    if(blendMode != paz::BlendMode::Disable)
    {
        [[pipelineDescriptor colorAttachments][0] setBlendingEnabled:YES];
        [[pipelineDescriptor colorAttachments][0] setRgbBlendOperation:
            MTLBlendOperationAdd];
        [[pipelineDescriptor colorAttachments][0] setAlphaBlendOperation:
            MTLBlendOperationAdd];
        if(blendMode == paz::BlendMode::Additive)
        {
                [[pipelineDescriptor colorAttachments][0]
                    setSourceRGBBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][0]
                    setSourceAlphaBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][0]
                    setDestinationRGBBlendFactor:MTLBlendFactorOne];
                [[pipelineDescriptor colorAttachments][0]
                    setDestinationAlphaBlendFactor:MTLBlendFactorOne];
        }
        else if(blendMode == paz::BlendMode::Blend)
        {
            [[pipelineDescriptor colorAttachments][0] setSourceRGBBlendFactor:
                MTLBlendFactorSourceAlpha];
            [[pipelineDescriptor colorAttachments][0] setSourceAlphaBlendFactor:
                MTLBlendFactorSourceAlpha];
            [[pipelineDescriptor colorAttachments][0]
                setDestinationRGBBlendFactor:MTLBlendFactorOneMinusSourceAlpha];
            [[pipelineDescriptor colorAttachments][0]
                setDestinationAlphaBlendFactor:
                MTLBlendFactorOneMinusSourceAlpha];
        }
        // ...
        else
        {
            throw std::runtime_error("Invalid blending function.");
        }
    }
    [pipelineDescriptor setDepthAttachmentPixelFormat:[[VIEW_CONTROLLER mtkView]
        depthStencilPixelFormat]];
    _data->_pipelineState = create(pipelineDescriptor, _data->_vertexArgs,
        _data->_fragmentArgs, _data->_vertexAttributeStrides);
    [pipelineDescriptor release];
}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    [RENDERER ensureCommandBuffer];

    MTLRenderPassDescriptor* renderPassDescriptor;
    if(_data->_fbo)
    {
        renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
        if(!renderPassDescriptor)
        {
            throw std::runtime_error("Failed to create render pass descriptor"
                ".");
        }

        for(std::size_t i = 0; i < _data->_fbo->_colorAttachments.size(); ++i)
        {
            [[renderPassDescriptor colorAttachments][i] setTexture:
                static_cast<id<MTLTexture>>(_data->_fbo->_colorAttachments[i]->
                _texture)];
            if(colorLoadActions.empty() || colorLoadActions[i] == LoadAction::
                DontCare)
            {
                [[renderPassDescriptor colorAttachments][i] setLoadAction:
                    MTLLoadActionDontCare];
            }
            else if(colorLoadActions[i] == LoadAction::Clear)
            {
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
                throw std::runtime_error("Invalid color attachment load action."
                    );
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
            else if(depthLoadAction == LoadAction::Clear)
            {
                [[renderPassDescriptor depthAttachment] setClearDepth:1.0];
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
                throw std::runtime_error("Invalid depth attachment load action."
                    );
            }
            [[renderPassDescriptor depthAttachment] setStoreAction:
                MTLStoreActionStore];
        }
    }
    else // Output pass
    {
        renderPassDescriptor = [RENDERER currentRenderPassDescriptor];
        if(!renderPassDescriptor)
        {
            throw std::runtime_error("Failed to grab current render pass descri"
                "ptor.");
        }

        if(colorLoadActions.empty() || colorLoadActions[0] == LoadAction::
            DontCare)
        {
            [[renderPassDescriptor colorAttachments][0] setLoadAction:
                MTLLoadActionDontCare];
        }
        else if(colorLoadActions[0] == LoadAction::Clear)
        {
            [[renderPassDescriptor colorAttachments][0] setLoadAction:
                MTLLoadActionClear];
        }
        else if(colorLoadActions[0] == LoadAction::Load)
        {
            [[renderPassDescriptor colorAttachments][0] setLoadAction:
                MTLLoadActionLoad];
        }
        else
        {
            throw std::runtime_error("Invalid color attachment load action.");
        }

        if(depthLoadAction == LoadAction::DontCare)
        {
            [[renderPassDescriptor depthAttachment] setLoadAction:
                MTLLoadActionDontCare];
        }
        else if(depthLoadAction == LoadAction::Clear)
        {
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
    }

    _data->_renderEncoder = [[RENDERER commandBuffer]
        renderCommandEncoderWithDescriptor:renderPassDescriptor];

    if(_data->_fbo)
    {
        [renderPassDescriptor release];

        if(_data->_fbo->_width)
        {
            [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
                setViewport:{0., 0., static_cast<double>(_data->_fbo->_width),
                static_cast<double>(_data->_fbo->_height), 0., 1.}];
        }
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
    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        setDepthStencilState:depth_stencil_state(mode)];
}

void paz::RenderPass::end()
{
    [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
        endEncoding];
    _data->_renderEncoder = nullptr;
    if(_data->_fbo)
    {
        for(auto n : _data->_fbo->_colorAttachments)
        {
            n->ensureMipmaps();
        }
        if(_data->_fbo->_depthStencilAttachment)
        {
            _data->_fbo->_depthStencilAttachment->ensureMipmaps();
        }
    }
}

void paz::RenderPass::cull(CullMode mode)
{
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
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y)
{
    std::array<int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
{
    std::array<int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w)
{
    std::array<int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size)
{
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:sizeof(int)*size atIndex:_data->_vertexArgs.
            at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:sizeof(int)*size atIndex:_data->
            _fragmentArgs.at(name)];
    }
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x)
{
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y)
{
    std::array<unsigned int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z)
{
    std::array<unsigned int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w)
{
    std::array<unsigned int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size)
{
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:sizeof(unsigned int)*size atIndex:_data->
            _vertexArgs.at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:sizeof(unsigned int)*size atIndex:_data->
            _fragmentArgs.at(name)];
    }
}

void paz::RenderPass::uniform(const std::string& name, float x)
{
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y)
{
    std::array<float, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z)
{
    std::array<float, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w)
{
    std::array<float, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size)
{
    if(_data->_vertexArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBytes:x length:sizeof(float)*size atIndex:_data->
            _vertexArgs.at(name)];
    }
    if(_data->_fragmentArgs.count(name))
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setFragmentBytes:x length:sizeof(float)*size atIndex:_data->
            _fragmentArgs.at(name)];
    }
}

void paz::RenderPass::primitives(PrimitiveType type, const VertexBuffer&
    vertices)
{
    check_attributes(vertices._data->_buffers, _data->_vertexAttributeStrides,
        vertices._data->_numVertices);

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    if(type == PrimitiveType::Points)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawPrimitives:MTLPrimitiveTypePoint vertexStart:0 vertexCount:
            vertices._data->_numVertices];
    }
    else if(type == PrimitiveType::Lines)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:
            vertices._data->_numVertices];
    }
    else if(type == PrimitiveType::LineStrip)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawPrimitives:MTLPrimitiveTypeLineStrip vertexStart:0 vertexCount:
            vertices._data->_numVertices];
    }
    else if(type == PrimitiveType::LineLoop)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeLineStrip indexCount:vertices.
            _data->_numVertices + 1 indexType:IndexType indexBuffer:static_cast<
            id<MTLBuffer>>(vertices._data->_lineLoopIndices) indexBufferOffset:
            0];
    }
    else if(type == PrimitiveType::Triangles)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:
            vertices._data->_numVertices];
    }
    else if(type == PrimitiveType::TriangleStrip)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0
            vertexCount:vertices._data->_numVertices];
    }
    else if(type == PrimitiveType::TriangleFan)
    {
        const std::size_t n = vertices._data->_numVertices < 3 ? 0 : 3*vertices.
            _data->_numVertices - 6;
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:n
            indexType:IndexType indexBuffer:static_cast<id<MTLBuffer>>(vertices.
            _data->_triangleFanIndices) indexBufferOffset:0];
    }
    else
    {
        throw std::runtime_error("Invalid primitive type.");
    }
}

void paz::RenderPass::indexed(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices)
{
    check_attributes(vertices._data->_buffers, _data->_vertexAttributeStrides,
        vertices._data->_numVertices);

    for(std::size_t i = 0; i < vertices._data->_buffers.size(); ++i)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            setVertexBuffer:static_cast<id<MTLBuffer>>(vertices._data->_buffers[
            i]) offset:0 atIndex:i];
    }

    if(type == PrimitiveType::Points)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypePoint indexCount:indices.
            _data->_numIndices indexType:IndexType indexBuffer:static_cast<id<
            MTLBuffer>>(indices._data->_data) indexBufferOffset:0];
    }
    else if(type == PrimitiveType::Lines)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:indices.
            _data->_numIndices indexType:IndexType indexBuffer:static_cast<id<
            MTLBuffer>>(indices._data->_data) indexBufferOffset:0];
    }
    else if(type == PrimitiveType::LineStrip)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeLineStrip indexCount:indices.
            _data->_numIndices indexType:IndexType indexBuffer:static_cast<id<
            MTLBuffer>>(indices._data->_data) indexBufferOffset:0];
    }
    else if(type == PrimitiveType::LineLoop)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeLineStrip indexCount:indices.
            _data->_numIndices + 1 indexType:IndexType indexBuffer:static_cast<
            id<MTLBuffer>>(indices._data->_lineLoopIndices) indexBufferOffset:
            0];
    }
    else if(type == PrimitiveType::Triangles)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:indices.
            _data->_numIndices indexType:IndexType indexBuffer:static_cast<id<
            MTLBuffer>>(indices._data->_data) indexBufferOffset:0];
    }
    else if(type == PrimitiveType::TriangleStrip)
    {
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeTriangleStrip indexCount:
            indices._data->_numIndices indexType:IndexType indexBuffer:
            static_cast<id<MTLBuffer>>(indices._data->_data) indexBufferOffset:
            0];
    }
    else if(type == PrimitiveType::TriangleFan)
    {
        const std::size_t n = indices._data->_numIndices < 3 ? 0 : 3*indices.
            _data->_numIndices - 6;
        [static_cast<id<MTLRenderCommandEncoder>>(_data->_renderEncoder)
            drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:n
            indexType:IndexType indexBuffer:static_cast<id<MTLBuffer>>(indices.
            _data->_triangleFanIndices) indexBufferOffset:0];
    }
    else
    {
        throw std::runtime_error("Invalid primitive type.");
    }
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
    Framebuffer temp;
    temp._data = _data->_fbo;
    return temp;
}

#endif
