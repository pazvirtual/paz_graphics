#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "window.hpp"
#import <MetalKit/MetalKit.h>

#define APP_DELEGATE (AppDelegate*)[NSApp delegate]
#define VIEW_CONTROLLER (ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController]
#define RENDERER (Renderer*)[(ViewController*)[[(AppDelegate*)[NSApp delegate] \
    window] contentViewController] renderer]

static bool CursorDisabled = false;

namespace paz
{
    struct Initializer
    {
        Initializer();
        ~Initializer();
    };
}

static paz::Initializer Initializer;

static std::function<void(void)> Draw = [](){};

static std::chrono::time_point<std::chrono::steady_clock> FrameStart;
static double CurFrameTime = 1./60.;

static std::unordered_set<paz::ColorTarget*> ColorTargets;
static std::unordered_set<paz::DepthStencilTarget*> DepthStencilTargets;

static CGPoint PrevOrigin;

paz::Initializer::~Initializer()
{
    [NSApp release];
}

paz::Initializer::Initializer()
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

void paz::Window::MakeFullscreen()
{
    if(!IsFullscreen())
    {
        [[[APP_DELEGATE window] contentView] enterFullScreenMode:[NSScreen
            mainScreen] withOptions:nil];
        PrevOrigin = [[APP_DELEGATE window] frame].origin;
        [[APP_DELEGATE window] setFrameOrigin:[[NSScreen mainScreen] frame].
            origin];
    }
}

void paz::Window::MakeWindowed()
{
    if(IsFullscreen())
    {
        [[[APP_DELEGATE window] contentView] exitFullScreenModeWithOptions:nil];
        [[APP_DELEGATE window] setFrameOrigin:PrevOrigin];
    }
}

// ...

bool paz::Window::IsFullscreen()
{
    return [[APP_DELEGATE window] contentView].inFullScreenMode;
}

int paz::Window::ViewportWidth()
{
    return [RENDERER viewportSize].width;
}

int paz::Window::ViewportHeight()
{
    return [RENDERER viewportSize].height;
}

int paz::Window::Width()
{
    return [RENDERER size].width;
}

int paz::Window::Height()
{
    return [RENDERER size].height;
}

bool paz::Window::KeyDown(Key key)
{
    // The following ensures that very brief key presses are not missed when
    // checking `key_down()`.
    return [VIEW_CONTROLLER keyDown].at(static_cast<int>(key)) ||
        [VIEW_CONTROLLER keyPressed].at(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    return [VIEW_CONTROLLER keyPressed].at(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    return [VIEW_CONTROLLER keyReleased].at(static_cast<int>(key));
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
    if(CursorDisabled)
    {
        return [VIEW_CONTROLLER cursorOffset];
    }
    else
    {
        const NSPoint temp = [[APP_DELEGATE window]
            mouseLocationOutsideOfEventStream];
        return {temp.x, temp.y};
    }
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    return [VIEW_CONTROLLER scrollOffset];
}

void paz::Window::SetCursorMode(CursorMode mode)
{
    [APP_DELEGATE setCursorMode:mode];
    CursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    return [RENDERER aspectRatio];
}

void paz::resize_targets()
{
    for(auto& n : ColorTargets)
    {
        n->resize(paz::Window::ViewportWidth(), paz::Window::ViewportHeight());
    }
    for(auto& n : DepthStencilTargets)
    {
        n->resize(paz::Window::ViewportWidth(), paz::Window::ViewportHeight());
    }
}

void paz::Window::Loop(const std::function<void(void)>& draw)
{
    Draw = draw;
    FrameStart = std::chrono::steady_clock::now();
    while(![APP_DELEGATE done])
    {

        NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:
            [NSDate distantFuture] inMode:NSDefaultRunLoopMode dequeue:YES];
        [NSApp sendEvent:event];
        [NSApp updateWindows];
    }
}

void paz::draw_in_renderer()
{
    Draw();
    [VIEW_CONTROLLER resetEvents];
    const auto now = std::chrono::steady_clock::now();
    CurFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
}

void paz::Window::Quit()
{
    [NSApp terminate:nil];
}

double paz::Window::FrameTime()
{
    return CurFrameTime;
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

void paz::register_target(ColorTarget* target)
{
    if(ColorTargets.count(target))
    {
        throw std::logic_error("Color target has already been registered.");
    }
    ColorTargets.insert(target);
}

void paz::register_target(DepthStencilTarget* target)
{
    if(DepthStencilTargets.count(target))
    {
        throw std::logic_error("Depth/stencil target has already been registere"
            "d.");
    }
    DepthStencilTargets.insert(target);
}

void paz::unregister_target(ColorTarget* target)
{
    if(!ColorTargets.count(target))
    {
        throw std::logic_error("Color target was not registered.");
    }
    ColorTargets.erase(target);
}

void paz::unregister_target(DepthStencilTarget* target)
{
    if(!DepthStencilTargets.count(target))
    {
        throw std::logic_error("Depth/stencil target was not registered.");
    }
    DepthStencilTargets.erase(target);
}

std::vector<float> paz::Window::PrintScreen()
{
    throw std::logic_error("NOT IMPLEMENTED");
}

#endif
