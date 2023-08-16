#ifndef PAZ_GRAPHICS_RENDERER_HH
#define PAZ_GRAPHICS_RENDERER_HH

#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import <MetalKit/MetalKit.h>

@interface Renderer : NSObject<MTKViewDelegate>
@property(readonly) float aspectRatio;
@property(readonly) CGSize viewportSize;
@property(readonly) CGSize size;
@property(readonly, nonnull) id<MTLCommandBuffer> commandBuffer;
- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView*)view;
- (nullable MTLRenderPassDescriptor*)currentRenderPassDescriptor;
- (void)ensureCommandBuffer;
@end

#endif

#endif
