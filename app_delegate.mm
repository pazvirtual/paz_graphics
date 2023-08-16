#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include <unordered_map>
#include <sstream>
//#import <Kernel/IOKit/hidsystem/IOHIDUsageTables.h>

static void match_callback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
    paz::Gamepad g;
    g.device = device;
    // ...
    reinterpret_cast<std::vector<paz::Gamepad>*>(context)->push_back(g);
}

static void remove_callback(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
    auto* gamepads = reinterpret_cast<std::vector<paz::Gamepad>*>(context);
    for(auto it = gamepads->begin(); it != gamepads->end(); ++it)
    {
        if(it->device == device)
        {
            gamepads->erase(it);
            break;
        }
    }
}

@implementation AppDelegate
{
    std::unordered_map<std::string, std::string> _gamepadMappings;
    IOHIDManagerRef _hidManager;
    std::vector<paz::Gamepad> _gamepads;
}

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
        _cursorMode = paz::CursorMode::Normal;

        _screenRect = [[NSScreen mainScreen] frame];

        const CGFloat windowWidth = 0.5*_screenRect.size.width;
        const CGFloat windowHeight = 0.5*_screenRect.size.height;

        ViewController* viewController = [[ViewController alloc]
            initWithContentRect:NSMakeRect(0.0, 0.0, windowWidth,
            windowHeight)];
        _window = [NSWindow windowWithContentViewController:viewController];
        [viewController release];
        [_window makeFirstResponder:[_window contentViewController]];
        [static_cast<ViewController*>([_window contentViewController])
            resetEvents];

        [_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [_window setStyleMask:NSWindowStyleMaskTitled|
            NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable];
        [_window setDelegate:self];

        // Load gamepad mappings.
        {
            std::stringstream iss(paz::MacosControllerDb);
            std::string line;
            while(std::getline(iss, line))
            {
                if(line.empty() || line[0] == '#')
                {
                    continue;
                }
                const std::string guid = line.substr(0, 32);
                const std::string mapping = line.substr(33);
                _gamepadMappings[guid] = mapping;
            }
        }

        // Set callbacks to detect when a gamepad is connected or removed.
        _hidManager = IOHIDManagerCreate(kCFAllocatorDefault,
            kIOHIDOptionsTypeNone);

        CFMutableArrayRef matching = CFArrayCreateMutable(kCFAllocatorDefault,
            0, &kCFTypeArrayCallBacks);
        long page = kHIDPage_GenericDesktop;
        CFNumberRef pageRef = CFNumberCreate(kCFAllocatorDefault,
            kCFNumberLongType, &page);
        long usage = kHIDUsage_GD_GamePad;
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
        CFNumberRef usageRef = CFNumberCreate(kCFAllocatorDefault,
            kCFNumberLongType, &usage);
        CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsagePageKey), pageRef);
        CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsageKey), usageRef);
        CFArrayAppendValue(matching, dict);
        CFRelease(usageRef);
        CFRelease(dict);
        CFRelease(pageRef);
        IOHIDManagerSetDeviceMatchingMultiple(_hidManager, matching);
        CFRelease(matching);

        IOHIDManagerRegisterDeviceMatchingCallback(_hidManager, &match_callback,
            &_gamepads);
        IOHIDManagerRegisterDeviceRemovalCallback(_hidManager, &remove_callback,
            &_gamepads);
        IOHIDManagerScheduleWithRunLoop(_hidManager, CFRunLoopGetMain(),
            kCFRunLoopDefaultMode);
        IOHIDManagerOpen(_hidManager, kIOHIDOptionsTypeNone);

        // Get any initially-attached gamepads.
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
    }

    return self;
}

- (void)dealloc
{
    CFRelease(_hidManager);
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

- (void)setCursorMode:(paz::CursorMode)mode
{
    if(mode == paz::CursorMode::Normal)
    {
        if(_cursorMode == paz::CursorMode::Disable)
        {
            CGWarpMouseCursorPosition(_priorCursorPos);
        }
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor unhide];
    }
    else if(mode == paz::CursorMode::Hidden)
    {
        if(_cursorMode == paz::CursorMode::Disable)
        {
            CGWarpMouseCursorPosition(_priorCursorPos);
        }
        CGAssociateMouseAndMouseCursorPosition(true);
        [NSCursor hide];
    }
    else if(mode == paz::CursorMode::Disable)
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
    if(_cursorMode == paz::CursorMode::Hidden)
    {
        [NSCursor hide];
    }
    else if(_cursorMode == paz::CursorMode::Disable)
    {
        [NSCursor hide];
        [self centerCursor];
    }
    else
    {
        [NSCursor unhide];
    }
}

- (bool)getGamepadState:(paz::GamepadState*)state
{
    if(_gamepads.empty())
    {
        return false;
    }
    *state = _gamepads[0].state;
    return true;
}
@end

#endif
