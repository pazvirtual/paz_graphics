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

static std::chrono::time_point<std::chrono::steady_clock> FrameStart = std::
    chrono::steady_clock::now();
static double PrevFrameTime = 1./60.;

static std::unordered_set<void*> RenderTargets;

static CGPoint PrevOrigin;

static void poll_events()
{
    while(true)
    {
        NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:
            [NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
        if(!event)
        {
            break;
        }
        [NSApp sendEvent:event];
        [NSApp updateWindows];
    }
}

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
        poll_events();
    }
    @catch(NSException* e)
    {
        throw std::runtime_error("Failed to initialize NSApp: " + std::string(
            [[NSString stringWithFormat:@"%@", e] UTF8String]));
    }
}

void paz::resize_targets()
{
    for(auto n : RenderTargets)
    {
        reinterpret_cast<Texture::Data*>(n)->resize(Window::ViewportWidth(),
            Window::ViewportHeight());
    }
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

void paz::Window::SetTitle(const std::string& title)
{
    initialize();

    [[APP_DELEGATE window] setTitle:[NSString stringWithUTF8String:title.
        c_str()]];
}

bool paz::Window::IsKeyWindow()
{
    initialize();

    return [APP_DELEGATE isFocus];
}

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

void paz::Window::Quit()
{
    initialize();

    [NSApp terminate:nil];
}

bool paz::Window::Done()
{
    initialize();

    return [APP_DELEGATE done];
}

void paz::Window::EndFrame()
{
    initialize();

    [[VIEW_CONTROLLER mtkView] draw];
    [VIEW_CONTROLLER resetEvents];
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
    poll_events();
}

double paz::Window::FrameTime()
{
    return PrevFrameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    initialize();

    const NSSize size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMinSize:size];
    auto frame = [[APP_DELEGATE window] frame];
    frame.size.width = std::max(size.width, frame.size.width);
    frame.size.height = std::max(size.height, frame.size.height);
    [[APP_DELEGATE window] setFrame:frame display:YES];
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    const NSSize size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMaxSize:size];
    auto frame = [[APP_DELEGATE window] frame];
    frame.size.width = std::min(size.width, frame.size.width);
    frame.size.height = std::min(size.height, frame.size.height);
    [[APP_DELEGATE window] setFrame:frame display:YES];
}

void paz::Window::MakeResizable()
{
    initialize();

    [[APP_DELEGATE window] setStyleMask:[[APP_DELEGATE window] styleMask]|
        NSWindowStyleMaskResizable];
}

void paz::Window::MakeNotResizable()
{
    initialize();

    [[APP_DELEGATE window] setStyleMask:[[APP_DELEGATE window] styleMask]&
        ~NSWindowStyleMaskResizable];
}

void paz::Window::Resize(int width, int height)
{
    initialize();

    auto frame = [[APP_DELEGATE window] frame];
    frame.size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setFrame:frame display:YES];
}

paz::Image<float, 3> paz::Window::PrintScreen()
{
    initialize();

    const int width = ViewportWidth();
    const int height = ViewportHeight();

    Image<std::uint8_t, 4> bgraFlipped(width, height);
    [[[[VIEW_CONTROLLER mtkView] currentDrawable] texture] getBytes:bgraFlipped.
        data() bytesPerRow:4*width fromRegion:MTLRegionMake2D(0, 0, width,
        height) mipmapLevel:0];

    Image<float, 3> rgb(ViewportWidth(), ViewportHeight());
    for(int y = 0; y < ViewportHeight(); ++y)
    {
        for(int x = 0; x < ViewportWidth(); ++x)
        {
            for(int i = 0; i < 3; ++i)
            {
                rgb[3*(width*y + x) + 2 - i] = bgraFlipped[4*(width*(height -
                    1 - y) + x) + i]/255.f;
            }
        }
    }

    return rgb;
}

#endif
