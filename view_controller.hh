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
@property(readonly) std::array<bool, paz::Window::NumKeys> keyDown;
@property(readonly) std::array<bool, paz::Window::NumKeys> keyPressed;
@property(readonly) std::array<bool, paz::Window::NumKeys> keyReleased;
@property(readonly) std::array<bool, paz::Window::NumMouseButtons> mouseDown;
@property(readonly) std::array<bool, paz::Window::NumMouseButtons> mousePressed;
@property(readonly) std::array<bool, paz::Window::NumMouseButtons>
    mouseReleased;
@property(readonly) std::pair<double, double> cursorOffset;
@property(readonly) std::pair<double, double> scrollOffset;
@property(retain, readonly) id<MTLDevice> device;
- (id)initWithContentRect:(CGRect)rect;
- (void)resetEvents;
@end

#endif

#endif
