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

    // This is required for `paz::Window::PrintScreen()`.
    [_mtkView setFramebufferOnly:NO];

    // This is required for on-demand rendering.
    [_mtkView setEnableSetNeedsDisplay:NO];
    [_mtkView setPaused:YES];

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
}
@end

#endif
