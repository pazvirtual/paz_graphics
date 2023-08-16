#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define APP_DELEGATE static_cast<AppDelegate*>([NSApp delegate])
#define VIEW_CONTROLLER static_cast<ViewController*>([[static_cast<\
    AppDelegate*>([NSApp delegate]) window] contentViewController])
#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])

static bool CursorDisabled = false;

namespace paz
{
    struct Initializer
    {
        Initializer();
        ~Initializer();
    };
}

void paz::initialize()
{
    static paz::Initializer initializer;
}

static std::function<void(void)> Draw = [](){};

static std::chrono::time_point<std::chrono::steady_clock> FrameStart;
static double CurFrameTime = 1./60.;

static std::unordered_set<void*> RenderTargets;

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
    initialize();

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
    initialize();

    if(IsFullscreen())
    {
        [[[APP_DELEGATE window] contentView] exitFullScreenModeWithOptions:nil];
        [[APP_DELEGATE window] setFrameOrigin:PrevOrigin];
    }
}

// ...

bool paz::Window::IsFullscreen()
{
    initialize();

    return [[APP_DELEGATE window] contentView].inFullScreenMode;
}

int paz::Window::ViewportWidth()
{
    initialize();

    return [RENDERER viewportSize].width;
}

int paz::Window::ViewportHeight()
{
    initialize();

    return [RENDERER viewportSize].height;
}

int paz::Window::Width()
{
    initialize();

    return [RENDERER size].width;
}

int paz::Window::Height()
{
    initialize();

    return [RENDERER size].height;
}

bool paz::Window::KeyDown(Key key)
{
    initialize();

    // The following ensures that very brief key presses are not missed when
    // checking `key_down()`.
    return [VIEW_CONTROLLER keyDown].at(static_cast<int>(key)) ||
        [VIEW_CONTROLLER keyPressed].at(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    initialize();

    return [VIEW_CONTROLLER keyPressed].at(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    initialize();

    return [VIEW_CONTROLLER keyReleased].at(static_cast<int>(key));
}

bool paz::Window::MouseDown(int button)
{
    initialize();

    // The following ensures that very brief mouse button presses are not missed
    // when checking `mouse_down()`.
    return [VIEW_CONTROLLER mouseDown].at(button) || [VIEW_CONTROLLER
        mousePressed].at(button);
}

bool paz::Window::MousePressed(int button)
{
    initialize();

    return [VIEW_CONTROLLER mousePressed].at(button);
}

bool paz::Window::MouseReleased(int button)
{
    initialize();

    return [VIEW_CONTROLLER mouseReleased].at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    initialize();

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
    initialize();

    return [VIEW_CONTROLLER scrollOffset];
}

void paz::Window::SetCursorMode(CursorMode mode)
{
    initialize();

    [APP_DELEGATE setCursorMode:mode];
    CursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    initialize();

    return [RENDERER aspectRatio];
}

void paz::resize_targets()
{
    for(auto n : RenderTargets)
    {
        reinterpret_cast<Texture::Data*>(n)->resize(Window::ViewportWidth(),
            Window::ViewportHeight());
    }
}

void paz::Window::Loop(const std::function<void(void)>& draw)
{
    initialize();

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
    initialize();

    [NSApp terminate:nil];
}

double paz::Window::FrameTime()
{
    return CurFrameTime;
}

// ...

void paz::Window::SetMinSize(int width, int height)
{
    initialize();

    const auto size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMinSize:size];
    auto frame = [[APP_DELEGATE window] frame];
    frame.size = size;
    [[APP_DELEGATE window] setFrame:frame display:YES];
}

void paz::register_target(void* target)
{
    if(RenderTargets.count(target))
    {
        throw std::logic_error("Render target has already been registered.");
    }
    RenderTargets.insert(target);
}

void paz::unregister_target(void* target)
{
    if(!RenderTargets.count(target))
    {
        throw std::logic_error("Render target was not registered.");
    }
    RenderTargets.erase(target);
}

paz::Image<float, 3> paz::Window::PrintScreen()
{
    throw std::logic_error("NOT IMPLEMENTED");
}

#endif
