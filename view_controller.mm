#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "view_controller.hh"
#include "keycodes.hpp"

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
    }

    return self;
}

- (void)dealloc
{
    [_renderer release];
    [_mtkView release];
    [_device release];
    [super dealloc];
}

- (void)loadView
{
    _device = MTLCreateSystemDefaultDevice();
    _mtkView = [[MTKView alloc] initWithFrame:_contentRect device:_device];

    [self setView:_mtkView];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    if(![_mtkView device])
    {
        throw std::runtime_error("Metal is not supported on this device.");
    }

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
    [_mtkView setClearColor:MTLClearColorMake(0., 0., 0., 0.)];
    [_mtkView setDepthStencilPixelFormat:MTLPixelFormatDepth32Float];

    [_renderer mtkView:_mtkView drawableSizeWillChange:[_mtkView drawableSize]];
}

- (void)mouseMoved:(NSEvent*)event
{
    _cursorOffset.first += [event deltaX];
    _cursorOffset.second -= [event deltaY];
}

- (void)mouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
    [self mouseMoved:event];
}

- (void)mouseDown:(NSEvent*)__unused event
{
     _mouseDown[0] = true;
     _mousePressed[0] = true;
}

- (void)mouseUp:(NSEvent*)__unused event
{
     _mouseDown[0] = false;
     _mouseReleased[0] = true;
}

- (void)rightMouseDown:(NSEvent*)__unused event
{
     _mouseDown[1] = true;
     _mousePressed[1] = true;
}

- (void)rightMouseUp:(NSEvent*)__unused event
{
     _mouseDown[1] = false;
     _mouseReleased[1] = true;
}

- (void)otherMouseDown:(NSEvent*)event
{
    _mouseDown[[event buttonNumber]] = true;
    _mousePressed[[event buttonNumber]] = true;
}

- (void)otherMouseUp:(NSEvent*)event
{
    _mouseDown[[event buttonNumber]] = false;
    _mouseReleased[[event buttonNumber]] = true;
}

- (void)keyDown:(NSEvent*)event
{
    if(![event isARepeat])
    {
        const paz::Window::Key k = paz::convert_keycode([event keyCode]);
        if(k == paz::Window::Key::Unknown)
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
    if(![event isARepeat])
    {
        const paz::Window::Key k = paz::convert_keycode([event keyCode]);
        if(k == paz::Window::Key::Unknown)
        {
            return;
        }

        const int i = static_cast<int>(k);
        _keyDown[i] = false;
        _keyReleased[i] = true;
    }
}

- (void)flagsChanged:(NSEvent*)event
{
    static const int shift = static_cast<int>(paz::Window::Key::LeftShift);
    static const int control = static_cast<int>(paz::Window::Key::LeftControl);
    static const int option = static_cast<int>(paz::Window::Key::LeftAlt);
    static const int command = static_cast<int>(paz::Window::Key::LeftSuper);

    if([event modifierFlags]&NSEventModifierFlagShift && !_keyDown[shift])
    {
        _keyDown[shift] = true;
        _keyPressed[shift] = true;
    }
    else if([event modifierFlags]|NSEventModifierFlagShift && _keyDown[shift])
    {
        _keyDown[shift] = false;
        _keyReleased[shift] = true;
    }

    if([event modifierFlags]&NSEventModifierFlagControl && !_keyDown[control])
    {
        _keyDown[control] = true;
        _keyPressed[control] = true;
    }
    else if([event modifierFlags]|NSEventModifierFlagControl && _keyDown[
        control])
    {
        _keyDown[control] = false;
        _keyReleased[control] = true;
    }

    if([event modifierFlags]&NSEventModifierFlagOption && !_keyDown[option])
    {
        _keyDown[option] = true;
        _keyPressed[option] = true;
    }
    else if([event modifierFlags]|NSEventModifierFlagOption && _keyDown[option])
    {
        _keyDown[option] = false;
        _keyReleased[option] = true;
    }

    if([event modifierFlags]&NSEventModifierFlagCommand && !_keyDown[command])
    {
        _keyDown[command] = true;
        _keyPressed[command] = true;
    }
    else if([event modifierFlags]|NSEventModifierFlagCommand && _keyDown[
        command])
    {
        _keyDown[command] = false;
        _keyReleased[command] = true;
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
}
@end

#endif
