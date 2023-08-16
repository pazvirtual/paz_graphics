#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#import <MetalKit/MetalKit.h>

#define VIEW_CONTROLLER (ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController]
#define RENDERER (Renderer*)[(ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController] renderer]
#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

#if 1
#define UNUSED_ARG return;
#else
#define UNUSED_ARG throw std::runtime_error("Shader does not take argument \"" \
    + name + "\".");
#endif

#define CASE(a, b, n) case MTLDataType##a: return {MTLVertexFormat##a, n* \
    sizeof(b)};

static MTLPrimitiveType primitive_type(paz::RenderPass::PrimitiveType t)
{
    //
    if(t == paz::RenderPass::PrimitiveType::Triangles)
    {
        return MTLPrimitiveTypeTriangle;
    }
    //

    throw std::runtime_error("Invalid primitive type.");
}

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

static id<MTLRenderPipelineState> create(const void* descriptor, std::
    unordered_map<std::string, int>& vertexArgs, std::unordered_map<std::string,
    int>& fragmentArgs)
{
    // Get vertex attributes.
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor
        vertexDescriptor];
    int idx = 0;
    for(id n in [[(MTLRenderPipelineDescriptor*)descriptor vertexFunction]
        stageInputAttributes])
    {
        const auto formatStride = vertex_format_stride([n attributeType]);
        [vertexDescriptor attributes][idx].format = formatStride.first;
        [vertexDescriptor attributes][idx].bufferIndex = idx;
        [vertexDescriptor attributes][idx].offset = 0;
        [vertexDescriptor layouts][idx].stride = formatStride.second;
        ++idx;
    }
    [(MTLRenderPipelineDescriptor*)descriptor setVertexDescriptor:
        vertexDescriptor];
    [vertexDescriptor release];

    // Get uniforms.
    MTLRenderPipelineReflection* reflection;
    NSError* error = nil;
    id<MTLRenderPipelineState> pipelineState = [DEVICE
        newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)
        descriptor options:MTLPipelineOptionArgumentInfo reflection:&reflection
        error:&error];
    [(MTLRenderPipelineDescriptor*)descriptor release];
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

paz::RenderPass::~RenderPass()
{
    if(_pipelineState)
    {
        [(id<MTLRenderPipelineState>)_pipelineState release];
        _pipelineState = nullptr;
    }
    _fbo = nullptr;
    _vertexArgs.clear();
    _fragmentArgs.clear();
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const Shader& shader,
    DepthTestMode depthTest)
{
    _fbo = &fbo;
    _depthTestMode = depthTest;
    MTLRenderPipelineDescriptor* pipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineDescriptor setVertexFunction:(id<MTLFunction>)shader._vert];
    [pipelineDescriptor setFragmentFunction:(id<MTLFunction>)shader._frag];
    for(std::size_t i = 0; i < _fbo->colorAttachments.size(); ++i)
    {
        [[pipelineDescriptor colorAttachments][i] setPixelFormat:
            [(id<MTLTexture>)_fbo->colorAttachments[i]->_texture pixelFormat]];
    }
    if(_fbo->depthAttachment)
    {
        [pipelineDescriptor setDepthAttachmentPixelFormat:[(id<MTLTexture>)
            _fbo->depthAttachment->_texture pixelFormat]];
    }
    _pipelineState = create(pipelineDescriptor, _vertexArgs, _fragmentArgs);
}

paz::RenderPass::RenderPass(const Shader& shader)
{
    _depthTestMode = DepthTestMode::Disable;
    MTLRenderPipelineDescriptor* pipelineDescriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    [pipelineDescriptor setVertexFunction:(id<MTLFunction>)shader._vert];
    [pipelineDescriptor setFragmentFunction:(id<MTLFunction>)shader._frag];
    [[pipelineDescriptor colorAttachments][0] setPixelFormat:[[VIEW_CONTROLLER
        mtkView] colorPixelFormat]];
    _pipelineState = create(pipelineDescriptor, _vertexArgs, _fragmentArgs);
}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    MTLRenderPassDescriptor* renderPassDescriptor;
    if(_fbo)
    {
        renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
        if(!renderPassDescriptor)
        {
            throw std::runtime_error("Failed to create render pass descriptor"
                ".");
        }

        for(std::size_t i = 0; i < _fbo->colorAttachments.size(); ++i)
        {
            [[renderPassDescriptor colorAttachments][i] setTexture:
                (id<MTLTexture>) _fbo->colorAttachments[i]->_texture];
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

        if(_fbo->depthAttachment)
        {
            [[renderPassDescriptor depthAttachment] setTexture:(id<MTLTexture>)
                _fbo->depthAttachment->_texture];
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
    }

    id<MTLDepthStencilState> depthStencilState = nil;
    if(_depthTestMode != DepthTestMode::Disable)
    {
        MTLDepthStencilDescriptor* depthStencilDescriptor =
            [[MTLDepthStencilDescriptor alloc] init];
        if(_depthTestMode == DepthTestMode::Never)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionNever];
        }
        else if(_depthTestMode == DepthTestMode::Less)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionLess];
        }
        else if(_depthTestMode == DepthTestMode::Equal)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionEqual];
        }
        else if(_depthTestMode == DepthTestMode::LessEqual)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionLessEqual];
        }
        else if(_depthTestMode == DepthTestMode::Greater)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionGreater];
        }
        else if(_depthTestMode == DepthTestMode::NotEqual)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionNotEqual];
        }
        else if(_depthTestMode == DepthTestMode::GreaterEqual)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionGreaterEqual];
        }
        else if(_depthTestMode == DepthTestMode::Always)
        {
            [depthStencilDescriptor setDepthCompareFunction:
                MTLCompareFunctionAlways];
        }
        else
        {
            throw std::runtime_error("Invalid depth testing function.");
        }
        [depthStencilDescriptor setDepthWriteEnabled:YES];
        depthStencilState = [DEVICE newDepthStencilStateWithDescriptor:
            depthStencilDescriptor];
        [depthStencilDescriptor release];
    }

    _renderEncoder = [[RENDERER commandBuffer]
        renderCommandEncoderWithDescriptor:renderPassDescriptor];

    if(_fbo)
    {
        [renderPassDescriptor release];
    }

    // Flip winding order to CCW to match OpenGL standard.
    [(id<MTLRenderCommandEncoder>)_renderEncoder setFrontFacingWinding:
        MTLWindingCounterClockwise];

    [(id<MTLRenderCommandEncoder>)_renderEncoder setRenderPipelineState:
        (id<MTLRenderPipelineState>)_pipelineState];
    if(depthStencilState)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setDepthStencilState:
            depthStencilState];
        [depthStencilState release];
    }
}

void paz::RenderPass::end()
{
    [(id<MTLRenderCommandEncoder>)_renderEncoder endEncoding];
    _renderEncoder = nullptr;
}

void paz::RenderPass::cull(CullMode mode) const
{
    if(mode == CullMode::Disable)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setCullMode:
            MTLCullModeNone];
    }
    else if(mode == CullMode::Front)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setCullMode:
            MTLCullModeFront];
    }
    else if(mode == CullMode::Back)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setCullMode:
            MTLCullModeBack];
    }
    else
    {
        throw std::runtime_error("Invalid culling mode.");
    }
}

void paz::RenderPass::read(const std::string& name, const Texture& tex) const
{
    bool used = false;
    if(_vertexArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexTexture:
            (id<MTLTexture>)tex._texture atIndex:_vertexArgs.at(name)];
        const std::string samplerName = name + "Sampler";
        if(!_vertexArgs.count(samplerName))
        {
            throw std::logic_error("Vertex function takes texture as argument b"
                "ut not its sampler.");
        }
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexSamplerState:
            (id<MTLSamplerState>)tex._sampler atIndex:_vertexArgs.at(
            samplerName)];
    }
    if(_fragmentArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setFragmentTexture:
            (id<MTLTexture>)tex._texture atIndex:_fragmentArgs.at(name)];
        const std::string samplerName = name + "Sampler";
        if(!_fragmentArgs.count(samplerName))
        {
            throw std::logic_error("Fragment function takes texture as argument"
                " but not its sampler.");
        }
        [(id<MTLRenderCommandEncoder>)_renderEncoder setFragmentSamplerState:
            (id<MTLSamplerState>)tex._sampler atIndex:_fragmentArgs.at(
            samplerName)];
    }
    if(!used)
    {
        UNUSED_ARG
    }
}

void paz::RenderPass::uniform(const std::string& name, int x) const
{
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, int x, int y) const
{
    std::array<int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
    const
{
    std::array<int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w) const
{
    std::array<int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const int* x, int n)
    const
{
    bool used = false;
    if(_vertexArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexBytes:x length:
            n*sizeof(int) atIndex:_vertexArgs.at(name)];
    }
    if(_fragmentArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setFragmentBytes:x length:
            n*sizeof(int) atIndex:_fragmentArgs.at(name)];
    }
    if(!used)
    {
        UNUSED_ARG
    }
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x) const
{
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y) const
{
    std::array<unsigned int, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z) const
{
    std::array<unsigned int, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w) const
{
    std::array<unsigned int, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    int n) const
{
    bool used = false;
    if(_vertexArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexBytes:x length:
            n*sizeof(unsigned int) atIndex:_vertexArgs.at(name)];
    }
    if(_fragmentArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setFragmentBytes:x length:
            n*sizeof(unsigned int) atIndex:_fragmentArgs.at(name)];
    }
    if(!used)
    {
        UNUSED_ARG
    }
}

void paz::RenderPass::uniform(const std::string& name, float x) const
{
    uniform(name, &x, 1);
}

void paz::RenderPass::uniform(const std::string& name, float x, float y) const
{
    std::array<float, 2> v = {x, y};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z) const
{
    std::array<float, 3> v = {x, y, z};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w) const
{
    std::array<float, 4> v = {x, y, z, w};
    uniform(name, v.data(), v.size());
}

void paz::RenderPass::uniform(const std::string& name, const float* x, int n)
    const
{
    bool used = false;
    if(_vertexArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexBytes:x length:
            n*sizeof(float) atIndex:_vertexArgs.at(name)];
    }
    if(_fragmentArgs.count(name))
    {
        used = true;
        [(id<MTLRenderCommandEncoder>)_renderEncoder setFragmentBytes:x length:
            n*sizeof(float) atIndex:_fragmentArgs.at(name)];
    }
    if(!used)
    {
        UNUSED_ARG
    }
}

void paz::RenderPass::primitives(PrimitiveType type, const VertexBuffer&
    vertices, int offset) const
{
    for(std::size_t i = 0; i < vertices._buffers.size(); ++i)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexBuffer:
            (id<MTLBuffer>)vertices._buffers[i] offset:0 atIndex:i];
    }
    [(id<MTLRenderCommandEncoder>)_renderEncoder drawPrimitives:primitive_type(
        type) vertexStart:offset vertexCount:vertices._numVertices];
}

void paz::RenderPass::indexed(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices, int offset) const
{
    for(std::size_t i = 0; i < vertices._buffers.size(); ++i)
    {
        [(id<MTLRenderCommandEncoder>)_renderEncoder setVertexBuffer:
            (id<MTLBuffer>)vertices._buffers[i] offset:0 atIndex:i];
    }
    MTLIndexType t;
    switch(sizeof(unsigned int))
    {
        case 2: t = MTLIndexTypeUInt16; break;
        case 4: t = MTLIndexTypeUInt32; break;
        default: throw std::runtime_error("Indices must be 16 or 32 bits.");
    }
    [(id<MTLRenderCommandEncoder>)_renderEncoder drawIndexedPrimitives:
        primitive_type(type) indexCount:indices._numIndices indexType:t
        indexBuffer:(id<MTLBuffer>)indices._data indexBufferOffset:offset];
}

#endif
