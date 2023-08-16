#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "renderer.hh"
#include "window.hpp"

static const std::string QuadSrc = 1 + R"===(
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
constexpr sampler texSampler(mag_filter::nearest, min_filter::nearest);
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
fragment float4 frag(Data in [[stage_in]], texture2d<float> tex [[texture(0)]])
{
    return tex.sample(texSampler, in.uv);
}
)===";

static constexpr std::array<float, 16> QuadPos =
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

        // Set up a render pipeline for scaling the final render.
        NSError* error = nil;
        _quadLib = [_device newLibraryWithSource:[NSString stringWithUTF8String:
            QuadSrc.c_str()] options:nil error:&error];
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
    if(paz::Window::DpiScale() == [[NSScreen mainScreen] backingScaleFactor])
    {
        id<MTLBlitCommandEncoder> blitEncoder = [_commandBuffer
            blitCommandEncoder];
        [blitEncoder copyFromTexture:tex sourceSlice:0 sourceLevel:0
            sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake([tex
            width], [tex height], 1) toTexture:[[_view currentDrawable] texture]
            destinationSlice:0 destinationLevel:0 destinationOrigin:
            MTLOriginMake(0, 0, 0)];
        [blitEncoder synchronizeTexture:tex slice:0 level:0];
        [blitEncoder endEncoding];
    }
    else
    {
        MTLRenderPassDescriptor* descriptor = [_view
            currentRenderPassDescriptor];
        if(!descriptor)
        {
            throw std::runtime_error("Failed to grab current render pass descri"
                "ptor.");
        }
        id<MTLRenderCommandEncoder> renderEncoder = [_commandBuffer
            renderCommandEncoderWithDescriptor:descriptor];
        [renderEncoder setRenderPipelineState:_pipelineState];
        [renderEncoder setFragmentTexture:tex atIndex:0];
        [renderEncoder setVertexBuffer:_quadPos offset:0 atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:
            0 vertexCount:4];
        [renderEncoder endEncoding];
    }
}
@end

#endif
