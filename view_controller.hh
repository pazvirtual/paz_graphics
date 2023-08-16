#ifndef PAZ_GRAPHICS_VIEW_CONTROLLER_HH
#define PAZ_GRAPHICS_VIEW_CONTROLLER_HH

#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "renderer.hh"
#import <AppKit/AppKit.h>
#import <MetalKit/MetalKit.h>
#include <array>

@interface ViewController : NSViewController
@property(retain, readonly) MTKView* mtkView;
@property(retain, readonly) Renderer* renderer;
@property(readonly) std::array<bool, paz::NumKeys> keyDown;
@property(readonly) std::array<bool, paz::NumKeys> keyPressed;
@property(readonly) std::array<bool, paz::NumKeys> keyReleased;
@property(readonly) std::array<bool, paz::NumMouseButtons> mouseDown;
@property(readonly) std::array<bool, paz::NumMouseButtons> mousePressed;
@property(readonly) std::array<bool, paz::NumMouseButtons> mouseReleased;
@property(readonly) std::pair<double, double> cursorOffset;
@property(readonly) std::pair<double, double> scrollOffset;
@property(retain, readonly) id<MTLDevice> device;
- (id)initWithContentRect:(CGRect)rect;
- (void)resetEvents;
@end

#endif

#endif
