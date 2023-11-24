#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include <unordered_map>
#include <sstream>

static void match_callback(void* context, IOReturn /* result */, void*
    /* sender */, IOHIDDeviceRef device)
{
    auto* gamepads = reinterpret_cast<std::vector<paz::Gamepad>*>(context);

    // Check if this device was already connected.
    for(const auto& n : *gamepads)
    {
        if(n.device == device)
        {
            return;
        }
    }

    // Get GUID to look up mapping.
    char name[256] = {'U', 'n', 'k', 'n', 'o', 'w', 'n', '\0'};
    std::uint32_t vendor = 0;
    std::uint32_t product = 0;
    std::uint32_t version = 0;
    {
        CFTypeRef prop;
        prop = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
        if(prop)
        {
            CFStringGetCString(static_cast<CFStringRef>(prop), name, sizeof(
                name), kCFStringEncodingUTF8);
        }
        prop = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
        if(prop)
        {
            CFNumberGetValue(static_cast<CFNumberRef>(prop),
                kCFNumberSInt32Type, &vendor);
        }
        prop = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
        if(prop)
        {
            CFNumberGetValue(static_cast<CFNumberRef>(prop),
                kCFNumberSInt32Type, &product);
        }
        prop = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVersionNumberKey));
        if(prop)
        {
            CFNumberGetValue(static_cast<CFNumberRef>(prop),
                kCFNumberSInt32Type, &version);
        }
    }
    char buf[33];
    if(vendor && product)
    {
        std::snprintf(buf, sizeof(buf), "03000000%02x%02x0000%02x%02x0000%02x%0"
            "2x0000", static_cast<std::uint8_t>(vendor), static_cast<std::
            uint8_t>(vendor >> 8), static_cast<std::uint8_t>(product),
            static_cast<std::uint8_t>(product >> 8), static_cast<std::uint8_t>(
            version), static_cast<std::uint8_t>(version >> 8));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "05000000%02x%02x%02x%02x%02x%02x%02x%0"
            "2x%02x%02x%02x00", name[0], name[1], name[2], name[3], name[4],
            name[5], name[6], name[7], name[8], name[9], name[10]);
    }
    std::string guid = buf;

    // Check if we have a mapping for this type of gamepad.
    if(!paz::gamepad_mappings().count(guid))
    {
        return;
    }

    std::vector<paz::GamepadElement> axes, buttons, hats;

    CFArrayRef elems = IOHIDDeviceCopyMatchingElements(device, NULL,
        kIOHIDOptionsTypeNone);
    for(CFIndex i = 0; i < CFArrayGetCount(elems); i++)
    {
        IOHIDElementRef native = static_cast<IOHIDElementRef>(const_cast<void*>(
            CFArrayGetValueAtIndex(elems, i)));
        if(CFGetTypeID(native) != IOHIDElementGetTypeID())
        {
            continue;
        }

        const IOHIDElementType type = IOHIDElementGetType(native);
        if(type != kIOHIDElementTypeInput_Axis && type !=
            kIOHIDElementTypeInput_Button && type !=
            kIOHIDElementTypeInput_Misc)
        {
            continue;
        }

        std::vector<paz::GamepadElement>* target = nullptr;

        const std::uint32_t usage = IOHIDElementGetUsage(native);
        const std::uint32_t page = IOHIDElementGetUsagePage(native);
        if(page == kHIDPage_GenericDesktop)
        {
            switch(usage)
            {
                case kHIDUsage_GD_X:
                case kHIDUsage_GD_Y:
                case kHIDUsage_GD_Z:
                case kHIDUsage_GD_Rx:
                case kHIDUsage_GD_Ry:
                case kHIDUsage_GD_Rz:
                case kHIDUsage_GD_Slider:
                case kHIDUsage_GD_Dial:
                case kHIDUsage_GD_Wheel:
                    target = &axes;
                    break;
                case kHIDUsage_GD_Hatswitch:
                    target = &hats;
                    break;
                case kHIDUsage_GD_DPadUp:
                case kHIDUsage_GD_DPadRight:
                case kHIDUsage_GD_DPadDown:
                case kHIDUsage_GD_DPadLeft:
                case kHIDUsage_GD_SystemMainMenu:
                case kHIDUsage_GD_Select:
                case kHIDUsage_GD_Start:
                    target = &buttons;
                    break;
            }
        }
        else if(page == kHIDPage_Simulation)
        {
            switch(usage)
            {
                case kHIDUsage_Sim_Accelerator:
                case kHIDUsage_Sim_Brake:
                case kHIDUsage_Sim_Throttle:
                case kHIDUsage_Sim_Rudder:
                case kHIDUsage_Sim_Steering:
                    target = &axes;
                    break;
            }
        }
        else if(page == kHIDPage_Button || page == kHIDPage_Consumer)
        {
            target = &buttons;
        }

        if(target)
        {
            target->push_back({native, usage, target->size(),
                IOHIDElementGetLogicalMin(native), IOHIDElementGetLogicalMax(
                native)});
        }
    }
    CFRelease(elems);

    std::sort(axes.begin(), axes.end());
    std::sort(buttons.begin(), buttons.end());
    std::sort(hats.begin(), hats.end());

    gamepads->push_back({device, guid, name, axes, buttons, hats});
}

static void remove_callback(void* context, IOReturn /* result */, void*
    /* sender */, IOHIDDeviceRef device)
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

static long get_element_val(const paz::Gamepad& g, const paz::GamepadElement& e)
{
    long val = 0;
    if(g.device)
    {
        IOHIDValueRef ref;
        if(IOHIDDeviceGetValue(g.device, e.native, &ref) == kIOReturnSuccess)
        {
            val = IOHIDValueGetIntegerValue(ref);
        }
    }
    return val;
}

@implementation AppDelegate
{
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
        [appMenuItem release];

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

        NSMenuItem* viewMenuItem = [[NSMenuItem alloc] initWithTitle:@"View"
            action:NULL keyEquivalent:@""];
        NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
        [viewMenuItem setSubmenu:viewMenu];
        [mainMenu addItem:viewMenuItem];
        [viewMenuItem release];

        [viewMenu release];

        NSMenuItem* windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window"
            action:NULL keyEquivalent:@""];
        NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        [windowMenuItem setSubmenu:windowMenu];
        [mainMenu addItem:windowMenuItem];
        [windowMenuItem release];

        [windowMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Minimize"
            action:@selector(performMiniaturize:) keyEquivalent:@"m"]];

        [windowMenu release];

        [mainMenu release];

        // Create window and set content view controller.
        _cursorMode = paz::CursorMode::Normal;

        _screenRect = [[NSScreen mainScreen] frame];

        const CGFloat windowWidth = 0.5*_screenRect.size.width;
        const CGFloat windowHeight = 0.5*_screenRect.size.height;

        ViewController* viewController = [[ViewController alloc]
            initWithContentRect:NSMakeRect(0., 0., windowWidth, windowHeight)];
        _window = [NSWindow windowWithContentViewController:viewController];
        [viewController release];
        [_window makeFirstResponder:[_window contentViewController]];
        [static_cast<ViewController*>([_window contentViewController])
            resetEvents];

        [_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [_window setStyleMask:NSWindowStyleMaskTitled|
            NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable];
        [_window setDelegate:self];

        // Set callbacks to detect when a gamepad is connected or removed.
        _hidManager = IOHIDManagerCreate(kCFAllocatorDefault,
            kIOHIDOptionsTypeNone);

        CFMutableArrayRef matching = CFArrayCreateMutable(kCFAllocatorDefault,
            0, &kCFTypeArrayCallBacks);
        long page = kHIDPage_GenericDesktop;
        CFNumberRef pageRef = CFNumberCreate(kCFAllocatorDefault,
            kCFNumberLongType, &page);
        static constexpr std::array<long, 3> usages =
        {
            kHIDUsage_GD_Joystick,
            kHIDUsage_GD_GamePad,
            kHIDUsage_GD_MultiAxisController
        };
        for(auto n : usages)
        {
            CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
                kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                &kCFTypeDictionaryValueCallBacks);
            CFNumberRef usageRef = CFNumberCreate(kCFAllocatorDefault,
                kCFNumberLongType, &n);
            CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsagePageKey),
                pageRef);
            CFDictionarySetValue(dict, CFSTR(kIOHIDDeviceUsageKey), usageRef);
            CFArrayAppendValue(matching, dict);
            CFRelease(usageRef);
            CFRelease(dict);
        }
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
    if(_hidManager)
    {
        CFRelease(_hidManager);
    }
    if(_window)
    {
        [_window release];
    }
    if(_appName)
    {
        [_appName release];
    }
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

- (void)windowDidEnterFullScreen:(NSNotification*)__unused notification
{
    _isFullscreen = true;
    [NSApp setPresentationOptions:NSApplicationPresentationHideDock|
        NSApplicationPresentationHideMenuBar];
}

- (void)windowDidExitFullScreen:(NSNotification*)__unused notification
{
    _isFullscreen = false;
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
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
    if(_cursorMode == mode)
    {
        return;
    }

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
        NSPoint pos = [_window mouseLocationOutsideOfEventStream];
        NSRect rect = NSMakeRect(pos.x, pos.y, 0, 0);
        rect = [_window convertRectToScreen:rect];
        _priorCursorPos.x = rect.origin.x;
        _priorCursorPos.y = _screenRect.size.height - rect.origin.y;
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

    // Poll axes.
    std::vector<double> axes(_gamepads[0].axes.size(), 0.);
    for(std::size_t i = 0; i < _gamepads[0].axes.size(); ++i)
    {
        const long raw = get_element_val(_gamepads[0], _gamepads[0].axes[i]);
        _gamepads[0].axes[i].min = std::min(_gamepads[0].axes[i].min, raw);
        _gamepads[0].axes[i].max = std::max(_gamepads[0].axes[i].max, raw);
        const long range = _gamepads[0].axes[i].max - _gamepads[0].axes[i].min;
        if(range)
        {
            axes[i] = 2.*(raw - _gamepads[0].axes[i].min)/range - 1.;
        }
    }

    // Poll buttons.
    std::vector<bool> buttons(_gamepads[0].buttons.size());
    for(std::size_t i = 0; i < _gamepads[0].buttons.size(); ++i)
    {
        buttons[i] = get_element_val(_gamepads[0], _gamepads[0].buttons[i]) -
            _gamepads[0].buttons[i].min > 0;
    }

    // Poll hats.
    std::vector<int> hats(_gamepads[0].hats.size());
    for(std::size_t i = 0; i < _gamepads[0].hats.size(); ++i)
    {
        static constexpr std::array<int, 9> hatStates =
        {
            1,   // up
            1|2, // right-up
            2,   // right
            2|4, // right-down
            4,   // down
            4|8, // left-down
            8,   // left
            1|8, // left-up
            0    // centered
        };
        long state = get_element_val(_gamepads[0], _gamepads[0].hats[i]) -
            _gamepads[0].hats[i].min;
        if(state < 0 || state > 8)
        {
            state = 8;
        }
        hats[i] = hatStates[state];
    }

    // Use mapping to get state.
    const auto& mapping = paz::gamepad_mappings().at(_gamepads[0].guid);
    for(int i = 0; i < paz::NumGamepadButtons; ++i)
    {
        const auto e = mapping.buttons[i];
        switch(e.type)
        {
            case paz::GamepadElementType::Axis:
            {
                const float val = axes[e.idx]*e.axisScale + e.axisOffset;
                if(e.axisOffset < 0. || (!e.axisOffset && e.axisScale > 0.))
                {
                    state->buttons[i] = val >= 0.;
                }
                else
                {
                    state->buttons[i] = val <= 0.;
                }
                break;
            }
            case paz::GamepadElementType::HatBit:
            {
                const unsigned int hat = e.idx >> 4;
                const unsigned int bit = e.idx&0xf;
                state->buttons[i] = hats[hat]&bit;
                break;
            }
            case paz::GamepadElementType::Button:
            {
                state->buttons[i] = buttons[e.idx];
                break;
            }
        }
    }
    for(int i = 0; i < 6; ++i)
    {
        const auto e = mapping.axes[i];
        switch(e.type)
        {
            case paz::GamepadElementType::Axis:
            {
                const double val = axes[e.idx]*e.axisScale + e.axisOffset;
                state->axes[i] = std::max(-1., std::min(1., val));
                break;
            }
            case paz::GamepadElementType::HatBit:
            {
                const unsigned int hat = e.idx >> 4;
                const unsigned int bit = e.idx&0xf;
                state->axes[i] = hats[hat]&bit ? 1. : 1.;
                break;
            }
            case paz::GamepadElementType::Button:
            {
                state->axes[i] = 2.*buttons[e.idx] - 1.;
                break;
            }
        }
    }

    return true;
}
@end

#endif
