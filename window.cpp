#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "keycodes.hpp"
#include "util.hpp"
#include "window.hpp"
#include "internal_data.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static GLFWwindow* WindowPtr;
static int WindowWidth;
static int WindowHeight;
static bool WindowIsKey;
static bool WindowIsFullscreen;
static int PrevX;
static int PrevY;
static int PrevHeight;
static int PrevWidth;
static int FboWidth;
static int FboHeight;
static float FboAspectRatio;
static int MinWidth = GLFW_DONT_CARE;
static int MinHeight = GLFW_DONT_CARE;
static int MaxWidth = GLFW_DONT_CARE;
static int MaxHeight = GLFW_DONT_CARE;

static std::array<bool, paz::NumKeys> KeyDown = {};
static std::array<bool, paz::NumKeys> KeyPressed = {};
static std::array<bool, paz::NumKeys> KeyReleased = {};
static std::array<bool, paz::NumMouseButtons> MouseDown = {};
static std::array<bool, paz::NumMouseButtons> MousePressed = {};
static std::array<bool, paz::NumMouseButtons> MouseReleased = {};
static std::pair<double, double> MousePos = {};
static std::pair<double, double> ScrollOffset = {};

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

static void key_callback(int key, int action)
{
    const paz::Key k = paz::convert_keycode(key);
    if(k == paz::Key::Unknown)
    {
        return;
    }

    const int i = static_cast<int>(k);
    if(action == GLFW_PRESS)
    {
        KeyDown[i] = true;
        KeyPressed[i] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        KeyDown[i] = false;
        KeyReleased[i] = true;
    }
}

static void mouse_button_callback(int button, int action)
{
    if(action == GLFW_PRESS)
    {
        MouseDown[button] = true;
        MousePressed[button] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        MouseDown[button] = false;
        MouseReleased[button] = true;
    }
}

static void cursor_position_callback(double xPos, double yPos)
{
    MousePos.first = xPos;
    MousePos.second = WindowHeight - yPos;
}

static void scroll_callback(double xOffset, double yOffset)
{
    ScrollOffset.first = xOffset;
    ScrollOffset.second = yOffset;
}

static void focus_callback(int focused)
{
    WindowIsKey = focused;
}

static void resize_callback(int width, int height)
{
    WindowWidth = width;
    WindowHeight = height;

    glfwGetFramebufferSize(WindowPtr, &FboWidth, &FboHeight);
    FboAspectRatio = static_cast<float>(FboWidth)/FboHeight;
    paz::resize_targets();
}

static void reset_events()
{
    if(CursorDisabled)
    {
        ::MousePos = {};
    }
    ::ScrollOffset = {};
    ::KeyPressed = {};
    ::KeyReleased = {};
    ::MousePressed = {};
    ::MouseReleased = {};
}

paz::Initializer::~Initializer()
{
    glfwTerminate();
}

paz::Initializer::Initializer()
{
    if(!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    // Set context hints.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, paz::GlMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, paz::GlMinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);

    // Get display size.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if(!monitor)
    {
        throw std::runtime_error("Failed to get primary monitor. You may be usi"
            "ng a remote shell"
#ifdef PAZ_LINUX
            " and need to set the `DISPLAY` environment variable.");
#else
            ".");
#endif
    }
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    const int displayWidth = videoMode->width;
    const int displayHeight = videoMode->height;

    // Set window size in screen coordinates (not pixels).
    WindowWidth = 0.5*displayWidth;
    WindowHeight = 0.5*displayHeight;//displayHeight - 200;

    // Create window and set as current context.
    WindowPtr = glfwCreateWindow(WindowWidth, WindowHeight,
        "PAZ_Graphics Window", nullptr, nullptr);
    WindowIsFullscreen = false;
    if(!WindowPtr)
    {
        throw std::runtime_error("Failed to open GLFW window. Your GPU may not "
            "be OpenGL " + std::to_string(paz::GlMajorVersion) + "." + std::
            to_string(paz::GlMinorVersion) + " compatible.");
    }
    glfwMakeContextCurrent(WindowPtr);
    glfwGetWindowPos(WindowPtr, &PrevX, &PrevY);

    // Load OpenGL functions.
    if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        throw std::runtime_error("Could not load OpenGL functions.");
    }

    // Get window size in pixels.
    glfwGetFramebufferSize(WindowPtr, &FboWidth, &FboHeight);
    FboAspectRatio = static_cast<float>(FboWidth)/FboHeight;

    // Activate vsync.
    glfwSwapInterval(1);

    // Activate depth clamping (for logarithmic depth).
    glEnable(GL_DEPTH_CLAMP);

    // Use raw mouse input when cursor is disabled.
    if(glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(WindowPtr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Set key, mouse and scroll callbacks.
    glfwSetKeyCallback(WindowPtr, [](GLFWwindow*, int key, int, int action, int)
        { key_callback(key, action); });
    glfwSetMouseButtonCallback(WindowPtr, [](GLFWwindow*, int button, int
        action, int){ mouse_button_callback(button, action); });
    glfwSetCursorPosCallback(WindowPtr, [](GLFWwindow*, double xPos, double
        yPos){ cursor_position_callback(xPos, yPos); });
    glfwSetScrollCallback(WindowPtr, [](GLFWwindow*, double xOffset, double
        yOffset){ scroll_callback(xOffset, yOffset); });
    glfwSetWindowFocusCallback(WindowPtr, [](GLFWwindow*, int focused){
        focus_callback(focused); });
    glfwSetWindowSizeCallback(WindowPtr, [](GLFWwindow*, int width, int height){
        resize_callback(width, height); });
}

void paz::Window::MakeFullscreen()
{
    initialize();

    if(!WindowIsFullscreen)
    {
        glfwGetWindowPos(WindowPtr, &PrevX, &PrevY);

        PrevWidth = WindowWidth;
        PrevHeight = WindowHeight;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(WindowPtr, monitor, 0, 0, videoMode->width,
            videoMode->height, GLFW_DONT_CARE);
        WindowIsFullscreen = true;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::MakeWindowed()
{
    initialize();

    if(WindowIsFullscreen)
    {
        glfwSetWindowMonitor(WindowPtr, nullptr, PrevX, PrevY, PrevWidth,
            PrevHeight, GLFW_DONT_CARE);
        WindowIsFullscreen = false;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::SetTitle(const std::string& title)
{
    initialize();

    glfwSetWindowTitle(WindowPtr, title.c_str());
}

bool paz::Window::IsKeyWindow()
{
    return WindowIsKey;
}

bool paz::Window::IsFullscreen()
{
    return WindowIsFullscreen;
}

int paz::Window::ViewportWidth()
{
    return FboWidth;
}

int paz::Window::ViewportHeight()
{
    return FboHeight;
}

int paz::Window::Width()
{
    return WindowWidth;
}

int paz::Window::Height()
{
    return WindowHeight;
}

bool paz::Window::KeyDown(Key key)
{
    return ::KeyDown.at(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    return ::KeyPressed.at(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    return ::KeyReleased.at(static_cast<int>(key));
}

bool paz::Window::MouseDown(int button)
{
    return ::MouseDown.at(button);
}

bool paz::Window::MousePressed(int button)
{
    return ::MousePressed.at(button);
}

bool paz::Window::MouseReleased(int button)
{
    return ::MouseReleased.at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    return ::MousePos;
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    return ::ScrollOffset;
}

void paz::Window::SetCursorMode(CursorMode mode)
{
    initialize();

    if(mode == CursorMode::Normal)
    {
        glfwSetInputMode(WindowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if(mode == CursorMode::Hidden)
    {
        glfwSetInputMode(WindowPtr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else if(mode == CursorMode::Disable)
    {
        glfwSetInputMode(WindowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        throw std::runtime_error("Unknown cursor mode.");
    }
    CursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    return FboAspectRatio;
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

    while(!glfwWindowShouldClose(WindowPtr))
    {
        draw();
        Commit();
    }
}

void paz::Window::Commit()
{
    initialize();

    glfwSwapBuffers(WindowPtr);
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
    glfwPollEvents();
    if(CursorDisabled)
    {
        glfwSetCursorPos(WindowPtr, 0, WindowHeight);
    }
}

void paz::Window::Quit()
{
    initialize();

    glfwSetWindowShouldClose(WindowPtr, GLFW_TRUE);
}

double paz::Window::FrameTime()
{
    return PrevFrameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    initialize();

    MinWidth = width;
    MinHeight = height;
    glfwSetWindowSizeLimits(WindowPtr, MinWidth, MinHeight, MaxWidth,
        MaxHeight);
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    MaxWidth = width;
    MaxHeight = height;
    glfwSetWindowSizeLimits(WindowPtr, MinWidth, MinHeight, MaxWidth,
        MaxHeight);
}

void paz::Window::MakeResizable()
{
    initialize();

    glfwSetWindowAttrib(WindowPtr, GLFW_RESIZABLE, GLFW_TRUE);
}

void paz::Window::MakeNotResizable()
{
    initialize();

    glfwSetWindowAttrib(WindowPtr, GLFW_RESIZABLE, GLFW_FALSE);
}

void paz::Window::Resize(int width, int height)
{
    initialize();

    if(MinWidth != GLFW_DONT_CARE)
    {
        width = std::max(width, MinWidth);
    }
    if(MaxWidth != GLFW_DONT_CARE)
    {
        width = std::min(width, MaxWidth);
    }
    if(MinHeight != GLFW_DONT_CARE)
    {
        height = std::max(height, MinHeight);
    }
    if(MaxHeight != GLFW_DONT_CARE)
    {
        height = std::min(height, MaxHeight);
    }
    glfwSetWindowSize(WindowPtr, width, height);
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
    initialize();

    Image<float, 3> image(ViewportWidth(), ViewportHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ViewportWidth(), ViewportHeight());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, ViewportWidth(), ViewportHeight(), GL_RGB, GL_FLOAT,
        image.data());
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error reading default framebuffer: " +
            gl_error(error) + ".");
    }
    return image;
}

#endif
