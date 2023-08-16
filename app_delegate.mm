#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.h"
#import "view_controller.h"

@implementation AppDelegate
// Initializer and deallocator.
- (id)initWithTitle:(std::string)title
{
    if(self = [super init])
    {
        _done = false;

        // Get application name.
        _appName = [[NSProcessInfo processInfo] processName];

        // Make sure the main window takes focus after initialization.
        [NSApp activateIgnoringOtherApps:YES];

        // Create main menu bar.
        NSMenu* mainMenu = [[NSMenu alloc] init];
        [NSApp setMainMenu:mainMenu];

        NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
        NSMenu* appMenu = [[NSMenu alloc] init];
        [appMenuItem setSubmenu:appMenu];
        [mainMenu addItem:appMenuItem];

        [appMenu addItem:[NSMenuItem separatorItem]];

        [appMenu addItem:[[NSMenuItem alloc] initWithTitle:[@"Hide "
            stringByAppendingString:_appName] action:@selector(hide:)
            keyEquivalent:@"h"]];

        NSMenuItem* hideOthers = [[NSMenuItem alloc] initWithTitle:@"Hide Other"
            "s" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
        [hideOthers setKeyEquivalentModifierMask:NSEventModifierFlagCommand|
            NSEventModifierFlagOption];
        [appMenu addItem:hideOthers];
        [hideOthers release];

        [appMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Show All" action:
            @selector(unhideAllApplications:) keyEquivalent:@""]];

        [appMenu addItem:[NSMenuItem separatorItem]];

        [appMenu addItem:[[NSMenuItem alloc] initWithTitle:[@"Quit "
            stringByAppendingString:_appName] action:@selector(terminate:)
            keyEquivalent:@"q"]];

        [appMenu release];
        [appMenuItem release];

        NSMenuItem* windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
            action:NULL keyEquivalent:@""];
        NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        [windowMenuItem setSubmenu:windowMenu];
        [mainMenu addItem:windowMenuItem];

        [windowMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Minimize"
            action:@selector(performMiniaturize:) keyEquivalent:@"m"]];

        [windowMenu release];
        [windowMenuItem release];

        [mainMenu release];

        // Create window and set content view controller.
        _cursorMode = paz::Window::CursorMode::Normal;

        _screenRect = [[NSScreen mainScreen] frame];

        const CGFloat windowWidth = 0.5*_screenRect.size.width;
        const CGFloat windowHeight = 0.5*_screenRect.size.height;

        ViewController* viewController = [[ViewController alloc]
            initWithContentRect:NSMakeRect(0.0, 0.0, windowWidth,
            windowHeight)];
        _window = [NSWindow windowWithContentViewController:viewController];
        [viewController release];
        [_window makeFirstResponder:[_window contentViewController]];
        [(ViewController*)[_window contentViewController] resetEvents];

        [_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [_window setStyleMask:NSWindowStyleMaskTitled|
            NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable];
        [_window setDelegate:self];
    }

    return self;
}

- (void)dealloc
{
    [_window release];
    [_appName release];
    [super dealloc];
}

// NSApplicationDelegate methods.
- (void)applicationWillFinishLaunching:(NSNotification*)__unused notification
{
    [_window makeKeyAndOrderFront:self];
}

- (void)applicationDidFinishLaunching:(NSNotification*)__unused notification
{
    [_window setAcceptsMouseMovedEvents:YES];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)
    __unused sender
{
    _done = true;

    return NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)__unused
    sender
{
    return YES;
}

// NSWindowDelegate methods.
- (void)windowDidBecomeKey:(NSNotification*)notification
{
    _isFocus = true;

    [self windowDidResize:notification];
}

- (void)windowDidResignKey:(NSNotification*)__unused notification
{
    _isFocus = false;
}

// Other methods.
- (void)centerCursor
{
    NSRect frame = [[_window contentView] bounds];
    frame = [[_window contentView] convertRect:frame toView:nil];
    frame = [_window convertRectToScreen:frame];
    CGWarpMouseCursorPosition(CGPointMake(frame.origin.x + 0.5*frame.size.width,
        _screenRect.size.height - frame.origin.y - 0.5*frame.size.height));
}

- (void)setCursorMode:(paz::Window::CursorMode)mode
{
    if(mode == paz::Window::CursorMode::Normal)
    {
        if(_cursorMode == paz::Window::CursorMode::Disable)
        {
            CGWarpMouseCursorPosition(_priorCursorPos);
        }
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor unhide];
    }
    else if(mode == paz::Window::CursorMode::Hidden)
    {
        if(_cursorMode == paz::Window::CursorMode::Disable)
        {
            CGWarpMouseCursorPosition(_priorCursorPos);
        }
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor hide];
    }
    else if(mode == paz::Window::CursorMode::Disable)
    {
        CGAssociateMouseAndMouseCursorPosition(false);
        [NSCursor hide];
        NSPoint mouseLocation = [NSEvent mouseLocation];
        _priorCursorPos = CGPointMake(mouseLocation.x, _screenRect.size.height -
            mouseLocation.y);
        [self centerCursor];
    }
    else
    {
        throw std::runtime_error("Unknown cursor mode.");
    }

    _cursorMode = mode;
}

- (void)windowDidChangeScreen:(NSNotification*)notification
{
    _screenRect = [[NSScreen mainScreen] frame];

    [self windowDidResize:notification];
}

- (void)windowDidResize:(NSNotification*)__unused notification
{
    // Re-hide/warp cursor.
    if(_cursorMode == paz::Window::CursorMode::Hidden)
    {
        [NSCursor hide];
    }
    else if(_cursorMode == paz::Window::CursorMode::Disable)
    {
        [NSCursor hide];
        [self centerCursor];
    }
    else
    {
        [NSCursor unhide];
    }
}
@end

#endif
