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
@property(readonly) std::array<bool, paz::NumGamepadButtons> gamepadDown;
@property(readonly) std::array<bool, paz::NumGamepadButtons> gamepadPressed;
@property(readonly) std::array<bool, paz::NumGamepadButtons> gamepadReleased;
@property(readonly) std::pair<double, double> gamepadLeftStick;
@property(readonly) std::pair<double, double> gamepadRightStick;
@property(readonly) double gamepadLeftTrigger;
@property(readonly) double gamepadRightTrigger;
@property(readonly) bool gamepadActive;
@property(readonly) bool mouseActive;
@property(retain, readonly) id<MTLDevice> device;
- (id)initWithContentRect:(CGRect)rect;
- (void)resetEvents;
- (void)pollGamepadState;
@end

#endif
