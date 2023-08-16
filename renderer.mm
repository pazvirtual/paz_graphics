#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "renderer.hh"
#include "window.hpp"
#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

@implementation Renderer
{
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    MTKView* _view;
}

- (instancetype)initWithMetalKitView:(MTKView*)view
{
    if(self = [super init])
    {
        _device = view.device;
        _commandQueue = [_device newCommandQueue];
        _view = view;
    }

    return self;
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

- (id<MTLTexture>)outputTex
{
    return [[_view currentDrawable] texture];
}
@end

#endif
