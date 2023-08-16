#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "view_controller.hh"
#import "app_delegate.hh"
#include "keycodes.hpp"
#import "gamepad_macos.hh"

@implementation ViewController
{
    CGRect _contentRect;
}

- (id)initWithContentRect:(CGRect)rect
{
    if(self = [super init])
    {
        _contentRect = rect;
        _keyDown = {};
        _keyPressed = {};
        _keyReleased = {};
        _mouseDown = {};
        _mousePressed = {};
        _mouseReleased = {};
        _cursorOffset = {};
        _scrollOffset = {};
        _gamepadDown = {};
        _gamepadPressed = {};
        _gamepadReleased = {};
        _gamepadLeftStick = {};
        _gamepadRightStick = {};
        _gamepadLeftTrigger = -1.;
        _gamepadRightTrigger = -1.;
        _gamepadActive = false;
        _mouseActive = false;
    }

    return self;
}

- (void)dealloc
{
    if(_renderer)
    {
        [_renderer release];
    }
    if(_mtkView)
    {
        [_mtkView release];
    }
    if(_device)
    {
        [_device release];
    }
    [super dealloc];
}

- (void)loadView
{
    _device = MTLCreateSystemDefaultDevice();
    if(!_device)
    {
        throw std::runtime_error("Metal is not supported on this device.");
    }

    _mtkView = [[MTKView alloc] initWithFrame:_contentRect device:_device];
    [self setView:_mtkView];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // This is required for blitting to screen.
    [_mtkView setFramebufferOnly:NO];

    // This is required for on-demand rendering.
    [_mtkView setEnableSetNeedsDisplay:NO];
    [_mtkView setPaused:YES];

    // Create renderer and set as delegate.
    @try
    {
        _renderer = [[Renderer alloc] initWithMetalKitView:_mtkView];
    }
    @catch(NSException* e)
    {
        throw std::runtime_error("Failed to initialize renderer: " + std::
            string([[NSString stringWithFormat:@"%@", e] UTF8String]));
    }
    if(!_renderer)
    {
        throw std::runtime_error("Failed to initialize renderer.");
    }

    [_mtkView setDelegate:_renderer];

    [_renderer mtkView:_mtkView drawableSizeWillChange:[_mtkView drawableSize]];
}

- (void)mouseMoved:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    _cursorOffset.first += [event deltaX];
    _cursorOffset.second -= [event deltaY];
}

- (void)mouseDragged:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    [self mouseMoved:event];
}

- (void)mouseDown:(NSEvent*)__unused event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[0] = true;
    _mousePressed[0] = true;
}

- (void)mouseUp:(NSEvent*)__unused event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[0] = false;
    _mouseReleased[0] = true;
}

- (void)rightMouseDown:(NSEvent*)__unused event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[1] = true;
    _mousePressed[1] = true;
}

- (void)rightMouseUp:(NSEvent*)__unused event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[1] = false;
    _mouseReleased[1] = true;
}

- (void)otherMouseDown:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[[event buttonNumber]] = true;
    _mousePressed[[event buttonNumber]] = true;
}

- (void)otherMouseUp:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    _mouseDown[[event buttonNumber]] = false;
    _mouseReleased[[event buttonNumber]] = true;
}

- (void)keyDown:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = false;

    if(![event isARepeat])
    {
        const paz::Key k = paz::convert_keycode([event keyCode]);
        if(k == paz::Key::Unknown)
        {
            return;
        }

        const int i = static_cast<int>(k);
        _keyDown[i] = true;
        _keyPressed[i] = true;
    }
}

- (void)keyUp:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = false;

    if(![event isARepeat])
    {
        const paz::Key k = paz::convert_keycode([event keyCode]);
        if(k == paz::Key::Unknown)
        {
            return;
        }

        const int i = static_cast<int>(k);
        _keyDown[i] = false;
        _keyReleased[i] = true;
    }
}

- (void)scrollWheel:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = true;

    _scrollOffset.first = [event scrollingDeltaX];
    _scrollOffset.second = [event scrollingDeltaY];

    if([event hasPreciseScrollingDeltas])
    {
        _scrollOffset.first *= 0.1;
        _scrollOffset.second *= 0.1;
    }
}

- (void)flagsChanged:(NSEvent*)event
{
    _gamepadActive = false;
    _mouseActive = false;

    static const int shiftIdx = static_cast<int>(paz::Key::LeftShift);
    static const int ctrlIdx = static_cast<int>(paz::Key::LeftControl);
    static const int optIdx = static_cast<int>(paz::Key::LeftAlt);
    static const int cmdIdx = static_cast<int>(paz::Key::LeftSuper);

    const bool shiftFlag = [event modifierFlags]&NSEventModifierFlagShift;
    const bool ctrlFlag = [event modifierFlags]&NSEventModifierFlagControl;
    const bool optFlag = [event modifierFlags]&NSEventModifierFlagOption;
    const bool cmdFlag = [event modifierFlags]&NSEventModifierFlagCommand;

    if(shiftFlag && !_keyDown[shiftIdx])
    {
        _keyDown[shiftIdx] = true;
        _keyPressed[shiftIdx] = true;
    }
    else if(!shiftFlag && _keyDown[shiftIdx])
    {
        _keyDown[shiftIdx] = false;
        _keyReleased[shiftIdx] = true;
    }

    if(ctrlFlag && !_keyDown[ctrlIdx])
    {
        _keyDown[ctrlIdx] = true;
        _keyPressed[ctrlIdx] = true;
    }
    else if(!ctrlFlag && _keyDown[ctrlIdx])
    {
        _keyDown[ctrlIdx] = false;
        _keyReleased[ctrlIdx] = true;
    }

    if(optFlag && !_keyDown[optIdx])
    {
        _keyDown[optIdx] = true;
        _keyPressed[optIdx] = true;
    }
    else if(!optFlag && _keyDown[optIdx])
    {
        _keyDown[optIdx] = false;
        _keyReleased[optIdx] = true;
    }

    if(cmdFlag && !_keyDown[cmdIdx])
    {
        _keyDown[cmdIdx] = true;
        _keyPressed[cmdIdx] = true;
    }
    else if(!cmdFlag && _keyDown[cmdIdx])
    {
        _keyDown[cmdIdx] = false;
        _keyReleased[cmdIdx] = true;
    }
}

- (void)resetEvents
{
    _cursorOffset = {};
    _scrollOffset = {};
    _keyPressed = {};
    _keyReleased = {};
    _mousePressed = {};
    _mouseReleased = {};
    _gamepadPressed = {};
    _gamepadReleased = {};
    _gamepadLeftStick = {};
    _gamepadRightStick = {};
    _gamepadLeftTrigger = -1.;
    _gamepadRightTrigger = -1.;
}

- (void)pollGamepadState
{
    paz::GamepadState state;
    if([static_cast<AppDelegate*>([NSApp delegate]) getGamepadState:&state])
    {
        for(int i = 0; i < paz::NumGamepadButtons; ++i)
        {
            if(state.buttons[i])
            {
                _gamepadActive = true;
                _mouseActive = false;
                if(!_gamepadDown[i])
                {
                    _gamepadPressed[i] = true;
                }
                _gamepadDown[i] = true;
            }
            else
            {
                if(_gamepadDown[i])
                {
                    _gamepadActive = true;
                    _mouseActive = false;
                    _gamepadReleased[i] = true;
                }
                _gamepadDown[i] = false;
            }
        }
        if(std::abs(state.axes[0]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.first = state.axes[0];
        }
        if(std::abs(state.axes[1]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.second = state.axes[1];
        }
        if(std::abs(state.axes[2]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.first = state.axes[2];
        }
        if(std::abs(state.axes[3]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.second = state.axes[3];
        }
        if(state.axes[4] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftTrigger = state.axes[4];
        }
        if(state.axes[5] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightTrigger = state.axes[5];
        }
    }
}
@end

#endif
