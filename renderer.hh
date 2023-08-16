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
@property(readonly) id<MTLCommandBuffer> _Nullable commandBuffer;
@property(readonly) id<MTLBuffer> _Nonnull skyVertices;
@property(readonly) id<MTLBuffer> _Nonnull quadVertices;
@property(readonly) id<MTLTexture> _Nonnull bayerTexture;
- (instancetype _Nonnull)initWithMetalKitView:(MTKView* _Nonnull)view;
- (MTLRenderPassDescriptor* _Nullable)currentRenderPassDescriptor;
@end

#endif

#endif
