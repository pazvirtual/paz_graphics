#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "renderer.hh"
#include "common.hpp"

static const char* QuadSrc = 1 + R"===(
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
constexpr sampler texSampler(mag_filter::nearest, min_filter::nearest);
uint hash(thread uint x)
{
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
uint hash(thread float2 v)
{
    return hash(v.x^hash(v.y));
}
float construct_float(thread uint m)
{
    constant uint ieeeMantissa = 0x007FFFFFu;
    constant uint ieeeOne = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    float f = as_type<float>(m);
    return f - 1.;
}
float unif_rand(thread vec2 v)
{
    return construct_float(hash(as_type<uint>(v)));
}
struct Data
{
    float4 pos [[position]];
    float2 uv;
};
vertex Data vert(uint idx [[vertex_id]], constant float2* pos [[buffer(0)]])
{
    Data out;
    out.pos = float4(pos[idx], 0., 1.);
    out.uv.x = 0.5*pos[idx].x + 0.5;
    out.uv.y = 0.5 - 0.5*pos[idx].y;
    return out;
}
fragment float4 frag(Data in [[stage_in]], texture2d<float> tex [[texture(0)]],
    constant float& gamma [[buffer(0)]], constant float& dither [[buffer(1)]])
{
    float4 col = tex.sample(texSampler, in.uv);
    col.rgb = pow(col.rgb, float3(1./gamma));
    col.rgb += dither*mix(-0.5/255., 0.5/255., unif_rand(in.uv));
    return col;
}
)===";

static constexpr std::array<float, 8> QuadPos =
{
     1, -1,
     1,  1,
    -1, -1,
    -1,  1
};

@implementation Renderer
{
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    MTKView* _view;
    id<MTLLibrary> _quadLib;
    id<MTLBuffer> _quadPos;
    id<MTLRenderPipelineState> _pipelineState;
}

- (instancetype)initWithMetalKitView:(MTKView*)view
{
    if(self = [super init])
    {
        _device = view.device;
        _commandQueue = [_device newCommandQueue];
        _view = view;
        _gamma = 2.2;
        _dither = false;

        // Set up a render pipeline for final gamma/scaling pass.
        NSError* error = nil;
        _quadLib = [_device newLibraryWithSource:[NSString stringWithUTF8String:
            QuadSrc] options:nil error:&error];
        if(!_quadLib)
        {
            throw std::runtime_error([[NSString stringWithFormat:@"Failed to cr"
                "eate shader library: %@", [error localizedDescription]]
                UTF8String]);
        }
        _quadPos = [_device newBufferWithBytes:QuadPos.data() length:QuadPos.
            size()*sizeof(float) options:MTLStorageModeShared];
        MTLRenderPipelineDescriptor* pipelineDescriptor =
            [[MTLRenderPipelineDescriptor alloc] init];;
        [[pipelineDescriptor colorAttachments][0] setPixelFormat:[_view
            colorPixelFormat]];
        id<MTLFunction> vert = [_quadLib newFunctionWithName:@"vert"];
        [pipelineDescriptor setVertexFunction:vert];
        [vert release];
        id<MTLFunction> frag = [_quadLib newFunctionWithName:@"frag"];
        [pipelineDescriptor setFragmentFunction:frag];
        [frag release];
        MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor
            vertexDescriptor];
        [vertexDescriptor attributes][0].format = MTLVertexFormatFloat2;
        [vertexDescriptor attributes][0].bufferIndex = 0;
        [vertexDescriptor attributes][0].offset = 0;
        [vertexDescriptor layouts][0].stride = 2*sizeof(float);
        [pipelineDescriptor setVertexDescriptor:vertexDescriptor];
        _pipelineState = [_device newRenderPipelineStateWithDescriptor:
            pipelineDescriptor error:&error];
        [pipelineDescriptor release];
        if(!_pipelineState)
        {
            throw std::runtime_error([[NSString stringWithFormat:@"Failed to cr"
                "eate pipeline state: %@", [error localizedDescription]]
                UTF8String]);
        }
    }

    return self;
}

- (void)dealloc
{
    if(_quadLib)
    {
        [_quadLib release];
    }
    if(_quadPos)
    {
        [_quadPos setPurgeableState:MTLPurgeableStateEmpty];
        [_quadPos release];
    }
    if(_pipelineState)
    {
        [_pipelineState release];
    }
    [super dealloc];
}

- (void)ensureCommandBuffer
{
    if(!_commandBuffer)
    {
        _commandBuffer = [_commandQueue commandBuffer];
    }
}

- (void)mtkView:(MTKView*)__unused view drawableSizeWillChange:(CGSize)size
{
    _viewportSize = size;

    const CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    _size.width = size.width/scale;
    _size.height = size.height/scale;

    _aspectRatio = size.width/size.height;
}

- (void)drawInMTKView:(MTKView*)__unused view
{
    @try
    {
        [_commandBuffer presentDrawable:[_view currentDrawable]];
        [_commandBuffer commit];
        [_commandBuffer waitUntilCompleted];
        _commandBuffer = nil;
    }
    @catch(NSException* e)
    {
        throw std::runtime_error("Rendering failed: " + std::string([[NSString
            stringWithFormat:@"%@", e] UTF8String]));
    }
}

- (MTLRenderPassDescriptor*)currentRenderPassDescriptor
{
    return [_view currentRenderPassDescriptor];
}

- (void)blitToScreen:(id<MTLTexture>)tex
{
    [self ensureCommandBuffer];
    MTLRenderPassDescriptor* descriptor = [_view currentRenderPassDescriptor];
    if(!descriptor)
    {
        throw std::runtime_error("Failed to grab current render pass descriptor"
            ".");
    }
    id<MTLRenderCommandEncoder> renderEncoder = [_commandBuffer
        renderCommandEncoderWithDescriptor:descriptor];
    [renderEncoder setRenderPipelineState:_pipelineState];
    [renderEncoder setFragmentTexture:tex atIndex:0];
    [renderEncoder setFragmentBytes:&_gamma length:sizeof(float) atIndex:0];
    const float f = _dither ? 1.f : 0.f;
    [renderEncoder setFragmentBytes:&f length:sizeof(float) atIndex:1];
    [renderEncoder setVertexBuffer:_quadPos offset:0 atIndex:0];
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0
        vertexCount:4];
    [renderEncoder endEncoding];
}
@end

#endif
