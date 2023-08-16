#ifndef APP_DELEGATE_HH
#define APP_DELEGATE_HH

#include "PAZ_Graphics"

#ifdef PAZ_MACOS

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
@property(readonly) paz::Window::CursorMode cursorMode;
- (id)initWithTitle:(std::string)title;
- (void)setCursorMode:(paz::Window::CursorMode)mode;
@end

#endif

#endif
