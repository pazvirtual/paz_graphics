#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.h"
#import "view_controller.h"
#import "renderer.h"
#import <MetalKit/MetalKit.h>

#define APP_DELEGATE (AppDelegate*)[NSApp delegate]
#define VIEW_CONTROLLER (ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController]
#define RENDERER (Renderer*)[(ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController] renderer]

std::function<void(void)> paz::Window::_draw = [](){};

std::chrono::time_point<std::chrono::steady_clock> paz::Window::_frameStart;
double paz::Window::_frameTime = 1./60.;

std::unordered_set<paz::RenderTarget*> paz::Window::_targets;

paz::Window::~Window()
{
    [NSApp release];
}

void paz::Window::Init()
{
    @try
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate* delegate = [[AppDelegate alloc] initWithTitle:"PAZ_Graphic"
            "s Window"];
        [NSApp setDelegate:delegate];
        [delegate release];
        [NSApp finishLaunching];
    }
    @catch(NSException* e)
    {
        throw std::runtime_error("Failed to initialize NSApp: " + std::string(
            [[NSString stringWithFormat:@"%@", e] UTF8String]));
    }
}

// ...

int paz::Window::Width()
{
    return [RENDERER viewportSize].x;
}

int paz::Window::Height()
{
    return [RENDERER viewportSize].y;
}

bool paz::Window::KeyDown(int key)
{
    // The following ensures that very brief key presses are not missed when
    // checking `key_down()`.
    return [VIEW_CONTROLLER keyDown].at(key) || [VIEW_CONTROLLER keyPressed].at(
        key);
}

bool paz::Window::KeyPressed(int key)
{
    return [VIEW_CONTROLLER keyPressed].at(key);
}

bool paz::Window::KeyReleased(int key)
{
    return [VIEW_CONTROLLER keyReleased].at(key);
}

bool paz::Window::KeyDown(Key key)
{
    return KeyDown(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    return KeyPressed(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    return KeyReleased(static_cast<int>(key));
}

bool paz::Window::MouseDown(unsigned int button)
{
    // The following ensures that very brief mouse button presses are not missed
    // when checking `mouse_down()`.
    return [VIEW_CONTROLLER mouseDown].at(button) || [VIEW_CONTROLLER
        mousePressed].at(button);
}

bool paz::Window::MousePressed(unsigned int button)
{
    return [VIEW_CONTROLLER mousePressed].at(button);
}

bool paz::Window::MouseReleased(unsigned int button)
{
    return [VIEW_CONTROLLER mouseReleased].at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    const NSPoint temp = [[APP_DELEGATE window]
        mouseLocationOutsideOfEventStream];
    return {temp.x, temp.y};
}

std::pair<double, double> paz::Window::MouseOffset()
{
    return [VIEW_CONTROLLER cursorOffset];
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    return [VIEW_CONTROLLER scrollOffset];
}

void paz::Window::ResetEvents()
{
    [VIEW_CONTROLLER resetEvents];
}

// ...

float paz::Window::AspectRatio()
{
    return [RENDERER aspectRatio];
}

void paz::Window::ResizeTargets()
{
    for(auto& n : _targets)
    {
        n->resize(Width(), Height());
    }
}

void paz::Window::LoopInternal()
{
    while(![APP_DELEGATE done])
    {

        NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:
            [NSDate distantFuture] inMode:NSDefaultRunLoopMode dequeue:YES];
        [NSApp sendEvent:event];
        [NSApp updateWindows];
    }
}
void paz::Window::DrawInRenderer()//TEMP
{
    _draw();
    ResetEvents();
    const auto now = std::chrono::steady_clock::now();
    _frameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        _frameStart).count()*1e-6;
    _frameStart = now;
}

void paz::Window::Quit()
{
    [NSApp terminate:nil];
}

double paz::Window::FrameTime()
{
    return _frameTime;
}

// ...

void paz::Window::SetMinSize(int width, int height)
{
    const auto size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMinSize:size];
    auto frame = [[APP_DELEGATE window] frame];
    frame.size = size;
    [[APP_DELEGATE window] setFrame:frame display:YES];
}

void paz::Window::RegisterTarget(RenderTarget* target)
{
    if(_targets.count(target))
    {
        throw std::logic_error("Rendering target has already been registered.");
    }
    _targets.insert(target);
}

void paz::Window::UnregisterTarget(RenderTarget* target)
{
    if(!_targets.count(target))
    {
        throw std::logic_error("Rendering target was not registered.");
    }
    _targets.erase(target);
}

#endif
