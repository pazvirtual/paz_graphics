#ifndef RENDERER_HH
#define RENDERER_HH

#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import <MetalKit/MetalKit.h>

@interface Renderer : NSObject<MTKViewDelegate>
@property(readonly) float aspectRatio;
@property(readonly) vector_uint2 viewportSize;
@property(readonly) id<MTLCommandBuffer> _Nullable commandBuffer;
@property(readonly) id<MTLBuffer> _Nonnull skyVertices;
@property(readonly) id<MTLBuffer> _Nonnull quadVertices;
@property(readonly) id<MTLTexture> _Nonnull bayerTexture;
- (instancetype _Nonnull)initWithMetalKitView:(MTKView* _Nonnull)view;
- (MTLRenderPassDescriptor* _Nullable)currentRenderPassDescriptor;
@end

#endif

#endif
