#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "keycodes.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static constexpr int MajorVersion = 4;
static constexpr int MinorVersion = 1;

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

static std::array<bool, paz::Window::NumKeys> KeyDown = {};
static std::array<bool, paz::Window::NumKeys> KeyPressed = {};
static std::array<bool, paz::Window::NumKeys> KeyReleased = {};
static std::array<bool, paz::Window::NumMouseButtons> MouseDown = {};
static std::array<bool, paz::Window::NumMouseButtons> MousePressed = {};
static std::array<bool, paz::Window::NumMouseButtons> MouseReleased = {};
static std::pair<double, double> MousePos = {};
static std::pair<double, double> MouseOffset = {};
static std::pair<double, double> ScrollOffset = {};

std::function<void(void)> paz::Window::_draw;

std::chrono::time_point<std::chrono::steady_clock> paz::Window::_frameStart;
double paz::Window::_frameTime = 1./60.;

std::unordered_set<paz::RenderTarget*> paz::Window::_targets;

static void key_callback(int key, int action)
{
    const paz::Window::Key k = paz::convert_keycode(key);
    if(k == paz::Window::Key::Unknown)
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
    yPos = WindowHeight - yPos;
    MouseOffset.first = xPos - MousePos.first;
    MouseOffset.second = yPos - MousePos.second;
    MousePos.first = xPos;
    MousePos.second = yPos;
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
    FboAspectRatio = (float)FboWidth/FboHeight;
    paz::Window::ResizeTargets();
}

paz::Window::~Window()
{
    glfwTerminate();
}

void paz::Window::Init()
{
    if(!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    // Set context hints.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, MajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, MinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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
            "be OpenGL " + std::to_string(MajorVersion) + "." + std::to_string(
            MinorVersion) + " compatible.");
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
    FboAspectRatio = (float)FboWidth/FboHeight;

    // Activate vsync.
    glfwSwapInterval(1);

    // Activate depth clamping (for logarithmic depth).
    glEnable(GL_DEPTH_CLAMP);

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

bool paz::Window::KeyDown(int key)
{
    return ::KeyDown.at(key);
}

bool paz::Window::KeyPressed(int key)
{
    return ::KeyPressed.at(key);
}

bool paz::Window::KeyReleased(int key)
{
    return ::KeyReleased.at(key);
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
    return ::MouseDown.at(button);
}

bool paz::Window::MousePressed(unsigned int button)
{
    return ::MousePressed.at(button);
}

bool paz::Window::MouseReleased(unsigned int button)
{
    return ::MouseReleased.at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    return ::MousePos;
}

std::pair<double, double> paz::Window::MouseOffset()
{
    return ::MouseOffset;
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    return ::ScrollOffset;
}

void paz::Window::ResetEvents()
{
    ::MouseOffset = {};
    ::ScrollOffset = {};
    ::KeyPressed = {};
    ::KeyReleased = {};
    ::MousePressed = {};
    ::MouseReleased = {};
}

void paz::Window::SetCursorMode(CursorMode mode)
{
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
}

float paz::Window::AspectRatio()
{
    return FboAspectRatio;
}

void paz::Window::ResizeTargets()
{
    for(auto& n : _targets)
    {
        n->resize(ViewportWidth(), ViewportHeight());
    }
}

void paz::Window::LoopInternal()
{
    _frameStart = std::chrono::steady_clock::now();
    while(!glfwWindowShouldClose(WindowPtr))
    {
        glfwPollEvents();
        _draw();
        glfwSwapBuffers(WindowPtr);
        ResetEvents();
        const auto now = std::chrono::steady_clock::now();
        _frameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
            _frameStart).count()*1e-6;
        _frameStart = now;
    }
}

void paz::Window::Quit()
{
    glfwSetWindowShouldClose(WindowPtr, GLFW_TRUE);
}

double paz::Window::FrameTime()
{
    return _frameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    // This sets limits and automatically resizes to meet them if windowed.
    glfwSetWindowSizeLimits(WindowPtr, width, height, GLFW_DONT_CARE,
        GLFW_DONT_CARE);
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
