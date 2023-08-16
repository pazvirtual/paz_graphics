#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "renderer.h"
#import <simd/simd.h>
#import <ModelIO/ModelIO.h>

@implementation Renderer
{
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
    bool _drewOnce;
    dispatch_semaphore_t _inFlightSemaphore;
    MTKView* _curView;
}

- (instancetype)initWithMetalKitView:(MTKView*)view
{
    if(self = [super init])
    {
        _drewOnce = false;

        _device = view.device;

        _inFlightSemaphore = dispatch_semaphore_create(3);

        _commandQueue = [_device newCommandQueue];
    }

    return self;
}

- (void)mtkView:(MTKView*)__unused view drawableSizeWillChange:(CGSize)size
{
    const CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    _viewportSize.x = size.width/scale;
    _viewportSize.y = size.height/scale;

    _aspectRatio = size.width/size.height;

    paz::Window::ResizeTargets();
}

- (void)drawInMTKView:(MTKView*)view
{
    _curView = view;
    @try
    {
        dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

        // Create a new command buffer.
        _commandBuffer = [_commandQueue commandBuffer];

        // Encode drawing commands.
        paz::Window::DrawInRenderer();

        // Render.
        [_commandBuffer presentDrawable:[view currentDrawable]];

        dispatch_semaphore_t semaphore = _inFlightSemaphore;
        [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer>)
            {
                dispatch_semaphore_signal(semaphore);
            }];

        [_commandBuffer commit];
    }
    @catch(NSException* e)
    {
        throw std::runtime_error("Rendering failed: " + std::string([[NSString
            stringWithFormat:@"%@", e] UTF8String]));
    }
}

- (MTLRenderPassDescriptor*)currentRenderPassDescriptor
{
    return [_curView currentRenderPassDescriptor];
}
@end

#endif
