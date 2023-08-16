#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "gamepad_macos.hh"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include <sys/param.h>

@interface AppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
@property(retain) NSWindow* window;
@property(readonly) NSRect screenRect;
@property(retain) NSString* appName;
@property(readonly) bool done;
@property(readonly) bool isFocus;
@property(readonly) CGPoint priorCursorPos;
@property(readonly) paz::CursorMode cursorMode;
- (id)initWithTitle:(std::string)title;
- (void)setCursorMode:(paz::CursorMode)mode;
- (bool)getGamepadState:(paz::GamepadState*)state;
@end

#endif
