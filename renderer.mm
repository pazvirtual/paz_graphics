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
    bool _drewOnce;
    MTKView* _curView;
}

- (instancetype)initWithMetalKitView:(MTKView*)view
{
    if(self = [super init])
    {
        _drewOnce = false;

        _device = view.device;

        _commandQueue = [_device newCommandQueue];

        // Create the first command buffer.
        _commandBuffer = [_commandQueue commandBuffer];
    }

    return self;
}

- (void)mtkView:(MTKView*)__unused view drawableSizeWillChange:(CGSize)size
{
    _viewportSize = size;

    const CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    _size.width = size.width/scale;
    _size.height = size.height/scale;

    _aspectRatio = size.width/size.height;

    paz::resize_targets();
}

- (void)drawInMTKView:(MTKView*)view
{
    _curView = view;
    @try
    {
        // Create a new command buffer.
        if(_drewOnce)
        {
            _commandBuffer = [_commandQueue commandBuffer];
        }

        // Encode drawing commands.
        paz::draw_in_renderer();

        // Render.
        [_commandBuffer presentDrawable:[view currentDrawable]];

        [_commandBuffer commit];

        [_commandBuffer waitUntilCompleted];

        _drewOnce = true;
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
