#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#import "renderer.hh"
#include "common.hpp"
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
static bool HidpiEnabled = true;

paz::Initializer& paz::initialize()
{
    static paz::Initializer initializer;
    return initializer;
}

static double PrevFrameTime = 1./60.;

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

    // Start recording frame time.
    frameStart = std::chrono::steady_clock::now();
}

void paz::resize_targets()
{
    static int width, height;
    if(width != Window::ViewportWidth() || height != Window::ViewportHeight())
    {
        width = Window::ViewportWidth();
        height = Window::ViewportHeight();
        for(auto n : initialize().renderTargets)
        {
            reinterpret_cast<Texture::Data*>(n)->resize(width, height);
        }
    }
}

void paz::register_target(void* t)
{
    if(initialize().renderTargets.count(t))
    {
        throw std::logic_error("Render target has already been registered.");
    }
    initialize().renderTargets.insert(t);
}

void paz::unregister_target(void* t)
{
    if(!initialize().renderTargets.count(t))
    {
        throw std::logic_error("Render target was not registered.");
    }
    initialize().renderTargets.erase(t);
}

void paz::Window::MakeFullscreen()
{
    initialize();

    if(!IsFullscreen())
    {
        [[APP_DELEGATE window] toggleFullScreen:nil];
    }
}

void paz::Window::MakeWindowed()
{
    initialize();

    if(IsFullscreen())
    {
        [[APP_DELEGATE window] toggleFullScreen:nil];
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

    return [APP_DELEGATE isFullscreen];
}

int paz::Window::ViewportWidth()
{
    initialize();

    const auto width = [RENDERER viewportSize].width;
    return ::HidpiEnabled ? width : std::round(width/[[APP_DELEGATE window]
        backingScaleFactor]);
}

int paz::Window::ViewportHeight()
{
    initialize();

    const auto height = [RENDERER viewportSize].height;
    return ::HidpiEnabled ? height : std::round(height/[[APP_DELEGATE window]
        backingScaleFactor]);
}

int paz::Window::Width()
{
    initialize();

    return std::round([RENDERER size].width);
}

int paz::Window::Height()
{
    initialize();

    return std::round([RENDERER size].height);
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

bool paz::Window::MouseDown(MouseButton button)
{
    initialize();

    // The following ensures that very brief mouse button presses are not missed
    // when checking `mouse_down()`.
    return [VIEW_CONTROLLER mouseDown].at(static_cast<int>(button)) ||
        [VIEW_CONTROLLER mousePressed].at(static_cast<int>(button));
}

bool paz::Window::MousePressed(MouseButton button)
{
    initialize();

    return [VIEW_CONTROLLER mousePressed].at(static_cast<int>(button));
}

bool paz::Window::MouseReleased(MouseButton button)
{
    initialize();

    return [VIEW_CONTROLLER mouseReleased].at(static_cast<int>(button));
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
        const NSPoint pos = [[APP_DELEGATE window]
            mouseLocationOutsideOfEventStream];
        return {pos.x, pos.y};
    }
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    initialize();

    return [VIEW_CONTROLLER scrollOffset];
}

bool paz::Window::GamepadDown(GamepadButton button)
{
    initialize();

    return [VIEW_CONTROLLER gamepadDown].at(static_cast<int>(button));
}

bool paz::Window::GamepadPressed(GamepadButton button)
{
    initialize();

    return [VIEW_CONTROLLER gamepadPressed].at(static_cast<int>(button));
}

bool paz::Window::GamepadReleased(GamepadButton button)
{
    initialize();

    return [VIEW_CONTROLLER gamepadReleased].at(static_cast<int>(button));
}

std::pair<double, double> paz::Window::GamepadLeftStick()
{
    initialize();

    return [VIEW_CONTROLLER gamepadLeftStick];
}

std::pair<double, double> paz::Window::GamepadRightStick()
{
    initialize();

    return [VIEW_CONTROLLER gamepadRightStick];
}

double paz::Window::GamepadLeftTrigger()
{
    initialize();

    return [VIEW_CONTROLLER gamepadLeftTrigger];
}

double paz::Window::GamepadRightTrigger()
{
    initialize();

    return [VIEW_CONTROLLER gamepadRightTrigger];
}

bool paz::Window::GamepadActive()
{
    initialize();

    return [VIEW_CONTROLLER gamepadActive];
}

bool paz::Window::MouseActive()
{
    initialize();

    return [VIEW_CONTROLLER mouseActive];
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

void paz::Window::PollEvents()
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
    [VIEW_CONTROLLER pollGamepadState];
}

void paz::Window::EndFrame()
{
    initialize();

    [RENDERER blitToScreen:static_cast<id<MTLTexture>>(final_framebuffer().
        colorAttachment(0)._data->_texture)];

    [[VIEW_CONTROLLER mtkView] draw];
    [VIEW_CONTROLLER resetEvents];
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        initialize().frameStart).count()*1e-6;
    initialize().frameStart = now;
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

paz::Image paz::Window::ReadPixels()
{
    initialize();

    if([RENDERER commandBuffer])
    {
        throw std::logic_error("Cannot read pixels before ending frame.");
    }

    const int width = ViewportWidth();
    const int height = ViewportHeight();

    std::vector<std::uint16_t> linearFlipped(4*width*height);
    [static_cast<id<MTLTexture>>(final_framebuffer().colorAttachment(0)._data->
        _texture) getBytes:linearFlipped.data() bytesPerRow:8*width fromRegion:
        MTLRegionMake2D(0, 0, width, height) mipmapLevel:0];

    Image rgba(ImageFormat::RGBA8UNorm_sRGB, width, height);
    static constexpr double d = 1./std::numeric_limits<std::uint16_t>::max();
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            const int yFlipped = height - 1 - y;
            for(int i = 0; i < 3; ++i)
            {
                rgba.bytes()[4*(width*y + x) + i] = to_srgb(linearFlipped[4*
                    (width*yFlipped + x) + i]*d);
            }
            rgba.bytes()[4*(width*y + x) + 3] = linearFlipped[4*(width*yFlipped
                + x) + 3]*d;
        }
    }

    return rgba;
}

float paz::Window::DpiScale()
{
    initialize();

    return ::HidpiEnabled ? [[APP_DELEGATE window] backingScaleFactor] : 1.;
}

float paz::Window::UiScale()
{
    initialize();

    return DpiScale();
}

void paz::Window::DisableHidpi()
{
    initialize();

    ::HidpiEnabled = false;
    resize_targets();
}

void paz::Window::EnableHidpi()
{
    initialize();

    ::HidpiEnabled = true;
    resize_targets();
}

bool paz::Window::HidpiEnabled()
{
    return ::HidpiEnabled;
}

bool paz::Window::HidpiSupported()
{
    return [[APP_DELEGATE window] backingScaleFactor] > 1.;
}

void paz::Window::SetGamma(float gamma)
{
    [RENDERER setGamma:gamma];
}

void paz::Window::DisableDithering()
{
    [RENDERER setDither:false];
}

void paz::Window::EnableDithering()
{
    [RENDERER setDither:true];
}

#endif
