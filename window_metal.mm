#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>
#include <chrono>

#define APP_DELEGATE static_cast<AppDelegate*>([NSApp delegate])
#define VIEW_CONTROLLER static_cast<ViewController*>([[static_cast<\
    AppDelegate*>([NSApp delegate]) window] contentViewController])
#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])

static bool CursorDisabled = false;

paz::Initializer& paz::initialize()
{
    static paz::Initializer initializer;
    return initializer;
}

static std::chrono::time_point<std::chrono::steady_clock> FrameStart = std::
    chrono::steady_clock::now();
static double PrevFrameTime = 1./60.;

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
    static int width, height;
    if(width != Window::ViewportWidth() || height != Window::ViewportHeight())
    {
        width = Window::ViewportWidth();
        height = Window::ViewportHeight();
        for(auto n : initialize()._renderTargets)
        {
            reinterpret_cast<Texture::Data*>(n)->resize(width, height);
        }
    }
}

void paz::register_target(void* t)
{
    if(initialize()._renderTargets.count(t))
    {
        throw std::logic_error("Render target has already been registered.");
    }
    initialize()._renderTargets.insert(t);
}

void paz::unregister_target(void* t)
{
    if(!initialize()._renderTargets.count(t))
    {
        throw std::logic_error("Render target was not registered.");
    }
    initialize()._renderTargets.erase(t);
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

    [RENDERER ensureCommandBuffer];
    id<MTLTexture> tex = static_cast<id<MTLTexture>>(final_framebuffer().
        colorAttachment(0)._data->_texture);
    id<MTLBlitCommandEncoder> blitEncoder = [[RENDERER commandBuffer]
        blitCommandEncoder];
    [blitEncoder copyFromTexture:tex sourceSlice:0 sourceLevel:0 sourceOrigin:
        MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(final_framebuffer().
        colorAttachment(0).width(), final_framebuffer().colorAttachment(0).
        height(), 1) toTexture:[RENDERER outputTex] destinationSlice:0
        destinationLevel:0 destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blitEncoder synchronizeTexture:tex slice:0 level:0];
    [blitEncoder endEncoding];

    [[VIEW_CONTROLLER mtkView] draw];
    [VIEW_CONTROLLER resetEvents];
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
    poll_events();
    resize_targets();
}

double paz::Window::FrameTime()
{
    initialize();

    return PrevFrameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    initialize();

    const NSSize size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMinSize:size];
    NSRect contentRect = [[APP_DELEGATE window] contentRectForFrameRect:
        [[APP_DELEGATE window] frame]];
    contentRect.size.width = std::max(size.width, contentRect.size.width);
    contentRect.size.height = std::max(size.height, contentRect.size.height);
    [[APP_DELEGATE window] setFrame:[[APP_DELEGATE window]
        frameRectForContentRect:contentRect] display:YES];

    resize_targets();
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    const NSSize size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setMaxSize:size];
    NSRect contentRect = [[APP_DELEGATE window] contentRectForFrameRect:
        [[APP_DELEGATE window] frame]];
    contentRect.size.width = std::min(size.width, contentRect.size.width);
    contentRect.size.height = std::min(size.height, contentRect.size.height);
    [[APP_DELEGATE window] setFrame:[[APP_DELEGATE window]
        frameRectForContentRect:contentRect] display:YES];

    resize_targets();
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

void paz::Window::Resize(int width, int height, bool viewportCoords)
{
    initialize();

    if(viewportCoords)
    {
        width = std::round(width/DpiScale());
        height = std::round(height/DpiScale());
    }

    NSRect contentRect = [[APP_DELEGATE window] contentRectForFrameRect:
        [[APP_DELEGATE window] frame]];
    contentRect.size = NSMakeSize(width, height);
    [[APP_DELEGATE window] setFrame:[[APP_DELEGATE window]
        frameRectForContentRect:contentRect] display:YES];

    resize_targets();
}

paz::Image<std::uint8_t, 4> paz::Window::ReadPixels()
{
    initialize();

    if([RENDERER commandBuffer])
    {
        throw std::logic_error("Cannot read pixels before ending frame.");
    }

    const int width = ViewportWidth();
    const int height = ViewportHeight();

    Image<std::uint8_t, 4> bgraFlipped(width, height);
    [static_cast<id<MTLTexture>>(final_framebuffer().colorAttachment(0)._data->
        _texture) getBytes:bgraFlipped.data() bytesPerRow:4*width fromRegion:
        MTLRegionMake2D(0, 0, width, height) mipmapLevel:0];

    Image<std::uint8_t, 4> rgba(width, height);
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            for(int i = 0; i < 4; ++i)
            {
                rgba[4*(width*y + x) + 3 - i] = bgraFlipped[4*(width*(height - 1
                    - y) + x) + i];
            }
        }
    }

    return rgba;
}

float paz::Window::DpiScale()
{
    return [[APP_DELEGATE window] backingScaleFactor];
}

float paz::Window::UiScale()
{
    return DpiScale();
}

#endif
