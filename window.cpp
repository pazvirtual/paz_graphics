#include "PAZ_Graphics"

#ifndef PAZ_MACOS

#include "keycodes.hpp"
#ifndef __gl_h_
#include "gl_core_4_1.h"
#endif
#include <GLFW/glfw3.h>

static constexpr int MajorVersion = 4;
static constexpr int MinorVersion = 1;

void* paz::Window::_window;
int paz::Window::_width;
int paz::Window::_height;
int paz::Window::_widthCoords;
int paz::Window::_heightCoords;
float paz::Window::_aspectRatio;
float paz::Window::_backingScaleFactor;
bool paz::Window::_isKeyWindow;
bool paz::Window::_isFullscreen;

int paz::Window::_windowX;
int paz::Window::_windowY;
int paz::Window::_windowWidth;
int paz::Window::_windowHeight;

std::array<bool, paz::Window::NumKeys> paz::Window::_keyDown = {};
std::array<bool, paz::Window::NumKeys> paz::Window::_keyPressed = {};
std::array<bool, paz::Window::NumKeys> paz::Window::_keyReleased = {};
std::array<bool, paz::Window::NumMouseButtons> paz::Window::_mouseDown = {};
std::array<bool, paz::Window::NumMouseButtons> paz::Window::_mousePressed = {};
std::array<bool, paz::Window::NumMouseButtons> paz::Window::_mouseReleased = {};

std::pair<double, double> paz::Window::_mousePos = {};
std::pair<double, double> paz::Window::_mouseOffset = {};
std::pair<double, double> paz::Window::_scrollOffset = {};

std::function<void(void)> paz::Window::_draw;

std::chrono::time_point<std::chrono::steady_clock> paz::Window::_frameStart;
double paz::Window::_frameTime = 1./60.;

std::unordered_set<paz::RenderTarget*> paz::Window::_targets;

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
    _windowWidth = 0.5*displayWidth;
    _windowHeight = 0.5*displayHeight;//displayHeight - 200;

    _widthCoords = _windowWidth;
    _heightCoords = _windowHeight;

    // Create window and set as current context.
    _window = glfwCreateWindow(_windowWidth, _windowHeight,
        "PAZ_Graphics Window", nullptr, nullptr);
    _isFullscreen = false;
    if(!_window)
    {
        throw std::runtime_error("Failed to open GLFW window. Your GPU may not "
            "be OpenGL " + std::to_string(MajorVersion) + "." + std::to_string(
            MinorVersion) + " compatible.");
    }
    glfwMakeContextCurrent((GLFWwindow*)_window);
    glfwGetWindowPos((GLFWwindow*)_window, &_windowX, &_windowY);

    // Load OpenGL functions.
    if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        throw std::runtime_error("Could not load OpenGL functions.");
    }

    // Get window size in pixels.
    glfwGetFramebufferSize((GLFWwindow*)_window, &_width, &_height);
    _aspectRatio = (float)_width/_height;

    // Detect display scale factor.
    float xScale, yScale;
    glfwGetWindowContentScale((GLFWwindow*)_window, &xScale, &yScale);
    _backingScaleFactor = std::min(xScale, yScale);

    // Activate vsync.
    glfwSwapInterval(1);

    // Activate depth clamping (for logarithmic depth).
    glEnable(GL_DEPTH_CLAMP);

    // Set key, mouse and scroll callbacks.
    glfwSetKeyCallback((GLFWwindow*)_window, [](GLFWwindow*, int key, int, int
        action, int){ Window::KeyCallback(key, action); });
    glfwSetMouseButtonCallback((GLFWwindow*)_window, [](GLFWwindow*, int button,
        int action, int){ Window::MouseButtonCallback(button, action); });
    glfwSetCursorPosCallback((GLFWwindow*)_window, [](GLFWwindow*, double xPos,
        double yPos){ Window::CursorPositionCallback(xPos, yPos); });
    glfwSetScrollCallback((GLFWwindow*)_window, [](GLFWwindow*, double xOffset,
        double yOffset){ Window::ScrollCallback(xOffset, yOffset); });
    glfwSetWindowFocusCallback((GLFWwindow*)_window, [](GLFWwindow*, int
        focused){ Window::FocusCallback(focused); });
    glfwSetWindowSizeCallback((GLFWwindow*)_window, [](GLFWwindow*, int width,
        int height){ Window::ResizeCallback(width, height); });
}

void paz::Window::MakeFullscreen()
{
    if(!_isFullscreen)
    {
        glfwGetWindowPos((GLFWwindow*)_window, &_windowX, &_windowY);
        _windowWidth = _widthCoords;
        _windowHeight = _heightCoords;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor((GLFWwindow*)_window, monitor, 0, 0, videoMode->
            width, videoMode->height, GLFW_DONT_CARE);
        _isFullscreen = true;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::MakeWindowed()
{
    if(_isFullscreen)
    {
        glfwSetWindowMonitor((GLFWwindow*)_window, nullptr, _windowX, _windowY,
            _windowWidth, _windowHeight, GLFW_DONT_CARE);
        _isFullscreen = false;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::SetTitle(const std::string& title)
{
    glfwSetWindowTitle((GLFWwindow*)_window, title.c_str());
}

bool paz::Window::IsKeyWindow()
{
    return _isKeyWindow;
}

bool paz::Window::IsFullscreen()
{
    return _isFullscreen;
}

int paz::Window::Width()
{
    return _width;
}

int paz::Window::Height()
{
    return _height;
}

bool paz::Window::KeyDown(int key)
{
    return _keyDown.at(key);
}

bool paz::Window::KeyPressed(int key)
{
    return _keyPressed.at(key);
}

bool paz::Window::KeyReleased(int key)
{
    return _keyReleased.at(key);
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
    return _mouseDown.at(button);
}

bool paz::Window::MousePressed(unsigned int button)
{
    return _mousePressed.at(button);
}

bool paz::Window::MouseReleased(unsigned int button)
{
    return _mouseReleased.at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    return _mousePos;
}

std::pair<double, double> paz::Window::MouseOffset()
{
    return _mouseOffset;
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    return _scrollOffset;
}

void paz::Window::ResetEvents()
{
    _mouseOffset = {};
    _scrollOffset = {};
    _keyPressed = {};
    _keyReleased = {};
    _mousePressed = {};
    _mouseReleased = {};
}

void paz::Window::SetCursorMode(CursorMode mode)
{
    if(mode == CursorMode::Normal)
    {
        glfwSetInputMode((GLFWwindow*)_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if(mode == CursorMode::Hidden)
    {
        glfwSetInputMode((GLFWwindow*)_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else if(mode == CursorMode::Disable)
    {
        glfwSetInputMode((GLFWwindow*)_window, GLFW_CURSOR,
            GLFW_CURSOR_DISABLED);
    }
    else
    {
        throw std::runtime_error("Unknown cursor mode.");
    }
}

float paz::Window::AspectRatio()
{
    return _aspectRatio;
}

void paz::Window::KeyCallback(int key, int action)
{
    const Key k = convert_keycode(key);
    if(k == Key::Unknown)
    {
        return;
    }

    const int i = static_cast<int>(k);
    if(action == GLFW_PRESS)
    {
        _keyDown[i] = true;
        _keyPressed[i] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        _keyDown[i] = false;
        _keyReleased[i] = true;
    }
}

void paz::Window::MouseButtonCallback(int button, int action)
{
    if(action == GLFW_PRESS)
    {
        _mouseDown[button] = true;
        _mousePressed[button] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        _mouseDown[button] = false;
        _mouseReleased[button] = true;
    }
}

void paz::Window::CursorPositionCallback(double xPos, double yPos)
{
    yPos = _heightCoords - yPos;
    _mouseOffset.first = xPos - _mousePos.first;
    _mouseOffset.second = yPos - _mousePos.second;
    _mousePos.first = xPos;
    _mousePos.second = yPos;
}

void paz::Window::ScrollCallback(double xOffset, double yOffset)
{
    _scrollOffset.first = xOffset;
    _scrollOffset.second = yOffset;
}

void paz::Window::FocusCallback(int focused)
{
    _isKeyWindow = focused;
}

void paz::Window::ResizeCallback(int width, int height)
{
    _widthCoords = width;
    _heightCoords = height;

    glfwGetFramebufferSize((GLFWwindow*)_window, &_width, &_height);
    _aspectRatio = (float)_width/_height;
    ResizeTargets();
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
    while(!glfwWindowShouldClose((GLFWwindow*)_window))
    {
        glfwPollEvents();
        _draw();
        glfwSwapBuffers((GLFWwindow*)_window);
        ResetEvents();
        const auto now = std::chrono::steady_clock::now();
        _frameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
            _frameStart).count()*1e-6;
        _frameStart = now;
    }
}

void paz::Window::Quit()
{
    glfwSetWindowShouldClose((GLFWwindow*)_window, GLFW_TRUE);
}

double paz::Window::FrameTime()
{
    return _frameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    // This sets limits and automatically resizes to meet them if windowed.
    glfwSetWindowSizeLimits((GLFWwindow*)_window, width, height, GLFW_DONT_CARE,
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
