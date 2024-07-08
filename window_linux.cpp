#include "detect_os.hpp"

#ifdef PAZ_LINUX

#include "PAZ_Graphics"
#include "render_pass.hpp"
#include "keycodes.hpp"
#include "common.hpp"
#include "internal_data.hpp"
#include "util_linux.hpp"
#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <chrono>

static const char* QuadVertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
out vec2 uv;
void main()
{
    gl_Position = vec4(pos, 0., 1.);
    uv = 0.5*pos + 0.5;
}
)===";
static const char* QuadFragSrc = 1 + R"===(
in vec2 uv;
uniform sampler2D tex;
uniform float gamma;
uniform float dither;
layout(location = 0) out vec4 color;
uint hash(in uint x)
{
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
uint hash(in uvec2 v)
{
    return hash(v.x^hash(v.y));
}
float construct_float(in uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    float f = uintBitsToFloat(m);
    return f - 1.;
}
float unif_rand(in vec2 v)
{
    return construct_float(hash(floatBitsToUint(v)));
}
void main()
{
    color = texture(tex, uv);
    color.rgb = pow(color.rgb, vec3(1./gamma));
    color.rgb += dither*mix(-0.5/255., 0.5/255., unif_rand(uv));
}
)===";

static constexpr std::array<float, 8> QuadPos =
{
     1, -1,
     1,  1,
    -1, -1,
    -1,  1
};

static GLFWwindow* _windowPtr;
static int _windowWidth;
static int _windowHeight;
static bool _windowIsKey;
static bool _windowIsFullscreen;
static int _prevX;
static int _prevY;
static int _prevHeight;
static int _prevWidth;
static int _fboWidth;
static int _fboHeight;
static float _fboAspectRatio;
static int _minWidth = GLFW_DONT_CARE;
static int _minHeight = GLFW_DONT_CARE;
static int _maxWidth = GLFW_DONT_CARE;
static int _maxHeight = GLFW_DONT_CARE;
static std::array<bool, paz::NumKeys> _keyDown;
static std::array<bool, paz::NumKeys> _keyPressed;
static std::array<bool, paz::NumKeys> _keyReleased;
static std::array<bool, paz::NumMouseButtons> _mouseDown;
static std::array<bool, paz::NumMouseButtons> _mousePressed;
static std::array<bool, paz::NumMouseButtons> _mouseReleased;
static std::pair<double, double> _mousePos;
static std::pair<double, double> _scrollOffset;
static std::array<bool, paz::NumGamepadButtons> _gamepadDown;
static std::array<bool, paz::NumGamepadButtons> _gamepadPressed;
static std::array<bool, paz::NumGamepadButtons> _gamepadReleased;
static std::pair<double, double> _gamepadLeftStick;
static std::pair<double, double> _gamepadRightStick;
static double _gamepadLeftTrigger = -1.;
static double _gamepadRightTrigger = -1.;
static bool _gamepadActive;
static bool _mouseActive;
static bool _cursorDisabled;
static bool _frameInProgress;
static bool _hidpiEnabled = true;
static float _gamma = 2.2;
static bool _dither;
static std::chrono::time_point<std::chrono::steady_clock> _frameStart;
static int _maxAnisotropy;

static double PrevFrameTime = 1./60.;

static void key_callback(int key, int action)
{
    _gamepadActive = false;
    _mouseActive = false;

    const paz::Key k = paz::convert_keycode(key);
    if(k == paz::Key::Unknown)
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

static void mouse_button_callback(int button, int action)
{
    _gamepadActive = false;
    _mouseActive = true;

    if(button < paz::NumMouseButtons)
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
}

static void cursor_position_callback(double xPos, double yPos)
{
    std::pair<double, double> newMousePos;
    if(_cursorDisabled)
    {
        newMousePos.first = xPos - _windowWidth/2;
        newMousePos.second = _windowHeight/2 - yPos;
    }
    else
    {
        newMousePos.first = xPos;
        newMousePos.second = _windowHeight - yPos;
    }

    if(newMousePos != _mousePos)
    {
        _gamepadActive = false;
        _mouseActive = true;

        _mousePos = newMousePos;
    }
}

static void scroll_callback(double xOffset, double yOffset)
{
    _gamepadActive = false;
    _mouseActive = true;

    _scrollOffset.first = xOffset;
    _scrollOffset.second = yOffset;
}

static void focus_callback(int focused)
{
    _windowIsKey = focused;
}

static void resize_callback(int width, int height)
{
    _windowWidth = width;
    _windowHeight = height;

    glfwGetFramebufferSize(_windowPtr, &_fboWidth, &_fboHeight);
    _fboAspectRatio = static_cast<float>(_fboWidth)/_fboHeight;
    paz::resize_targets();
}

static void reset_events()
{
    if(_cursorDisabled)
    {
        _mousePos = {};
    }
    _scrollOffset = {};
    _keyPressed = {};
    _keyReleased = {};
    _mousePressed = {};
    _mouseReleased = {};
    _gamepadPressed = {};
    _gamepadReleased = {};
    _gamepadLeftStick = {};
    _gamepadRightStick = {};
    _gamepadLeftTrigger = -1.;
    _gamepadRightTrigger = -1.;
}

paz::Initializer::~Initializer()
{
    glfwTerminate();
}

paz::Initializer::Initializer()
{
    if(!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW. You may be usi"
            "ng a remote shell and need to set the `DISPLAY` environment variab"
            "le.");
    }

    // Set context hints.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GlMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GlMinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Get display size.
    auto* monitor = glfwGetPrimaryMonitor();
    if(!monitor)
    {
        throw std::runtime_error("Failed to get primary monitor. You may be usi"
            "ng a remote shell and need to set the `DISPLAY` environment variab"
            "le.");
    }
    const auto* videoMode = glfwGetVideoMode(monitor);
    const int displayWidth = videoMode->width;
    const int displayHeight = videoMode->height;

    // Set window size in screen coordinates (not pixels).
    _windowWidth = 0.5*displayWidth;
    _windowHeight = 0.5*displayHeight;

    // Create window and set as current context.
    _windowPtr = glfwCreateWindow(_windowWidth, _windowHeight,
        "PAZ_Graphics Window", nullptr, nullptr);
    if(!_windowPtr)
    {
        throw std::runtime_error("Failed to create GLFW window. Your GPU may no"
            "t be OpenGL " + std::to_string(paz::GlMajorVersion) + "." + std::
            to_string(paz::GlMinorVersion) + " compatible.");
    }
    glfwMakeContextCurrent(_windowPtr);
    glfwGetWindowPos(_windowPtr, &_prevX, &_prevY);

    // Get window size in pixels.
    glfwGetFramebufferSize(_windowPtr, &_fboWidth, &_fboHeight);
    _fboAspectRatio = static_cast<float>(_fboWidth)/_fboHeight;

    // Load OpenGL functions.
    if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
    {
        throw std::runtime_error("Could not load OpenGL functions.");
    }

    // Activate vsync.
    glfwSwapInterval(1);

    // Activate depth clamping (for logarithmic depth).
    glEnable(GL_DEPTH_CLAMP);

    // Enable `gl_PointSize`.
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Use raw mouse input when cursor is disabled.
    if(glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(_windowPtr, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Set key, mouse and scroll callbacks.
    glfwSetKeyCallback(_windowPtr, [](GLFWwindow*, int key, int, int action,
        int){ key_callback(key, action); });
    glfwSetMouseButtonCallback(_windowPtr, [](GLFWwindow*, int button, int
        action, int){ mouse_button_callback(button, action); });
    glfwSetCursorPosCallback(_windowPtr, [](GLFWwindow*, double xPos, double
        yPos){ cursor_position_callback(xPos, yPos); });
    glfwSetScrollCallback(_windowPtr, [](GLFWwindow*, double xOffset, double
        yOffset){ scroll_callback(xOffset, yOffset); });
    glfwSetWindowFocusCallback(_windowPtr, [](GLFWwindow*, int focused){
        focus_callback(focused); });
    glfwSetWindowSizeCallback(_windowPtr, [](GLFWwindow*, int width, int
        height){ resize_callback(width, height); });

    // Get maximum supported anisotropy.
    {
        float temp;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &temp);
        _maxAnisotropy = std::max(1, static_cast<int>(std::round(temp)));
    }

    // Start recording frame time.
    _frameStart = std::chrono::steady_clock::now();
}

void paz::Window::MakeFullscreen()
{
    initialize();

    if(!_windowIsFullscreen)
    {
        glfwGetWindowPos(_windowPtr, &_prevX, &_prevY);

        _prevWidth = _windowWidth;
        _prevHeight = _windowHeight;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(_windowPtr, monitor, 0, 0, videoMode->width,
            videoMode->height, videoMode->refreshRate);
        _windowIsFullscreen = true;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::MakeWindowed()
{
    initialize();

    if(_windowIsFullscreen)
    {
        glfwSetWindowMonitor(_windowPtr, nullptr, _prevX, _prevY, _prevWidth,
            _prevHeight, GLFW_DONT_CARE);
        _windowIsFullscreen = false;

        // Keep vsync.
        glfwSwapInterval(1);
    }
}

void paz::Window::SetTitle(const std::string& title)
{
    initialize();

    glfwSetWindowTitle(_windowPtr, title.c_str());
}

bool paz::Window::IsKeyWindow()
{
    initialize();

    return _windowIsKey;
}

bool paz::Window::IsFullscreen()
{
    initialize();

    return _windowIsFullscreen;
}

int paz::Window::ViewportWidth()
{
    initialize();

    return _hidpiEnabled ? _fboWidth : _windowWidth;
}

int paz::Window::ViewportHeight()
{
    initialize();

    return _hidpiEnabled ? _fboHeight : _windowHeight;
}

int paz::Window::Width()
{
    initialize();

    return _windowWidth;
}

int paz::Window::Height()
{
    initialize();

    return _windowHeight;
}

bool paz::Window::KeyDown(Key key)
{
    initialize();

    return _keyDown.at(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    initialize();

    return _keyPressed.at(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    initialize();

    return _keyReleased.at(static_cast<int>(key));
}

bool paz::Window::MouseDown(MouseButton button)
{
    initialize();

    return _mouseDown.at(static_cast<int>(button));
}

bool paz::Window::MousePressed(MouseButton button)
{
    initialize();

    return _mousePressed.at(static_cast<int>(button));
}

bool paz::Window::MouseReleased(MouseButton button)
{
    initialize();

    return _mouseReleased.at(static_cast<int>(button));
}

std::pair<double, double> paz::Window::MousePos()
{
    initialize();

    if(_cursorDisabled)
    {
        return _mousePos;
    }
    else
    {
        double xPos, yPos;
        glfwGetCursorPos(_windowPtr, &xPos, &yPos);
        return {xPos, _windowHeight - yPos};
    }
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    initialize();

    return _scrollOffset;
}

bool paz::Window::GamepadDown(GamepadButton button)
{
    initialize();

    return _gamepadDown.at(static_cast<int>(button));
}

bool paz::Window::GamepadPressed(GamepadButton button)
{
    initialize();

    return _gamepadPressed.at(static_cast<int>(button));
}

bool paz::Window::GamepadReleased(GamepadButton button)
{
    initialize();

    return _gamepadReleased.at(static_cast<int>(button));
}

std::pair<double, double> paz::Window::GamepadLeftStick()
{
    initialize();

    return _gamepadLeftStick;
}

std::pair<double, double> paz::Window::GamepadRightStick()
{
    initialize();

    return _gamepadRightStick;
}

double paz::Window::GamepadLeftTrigger()
{
    initialize();

    return _gamepadLeftTrigger;
}

double paz::Window::GamepadRightTrigger()
{
    initialize();

    return _gamepadRightTrigger;
}

bool paz::Window::GamepadActive()
{
    initialize();

    return _gamepadActive;
}

bool paz::Window::MouseActive()
{
    initialize();

    return _mouseActive;
}

void paz::Window::SetCursorMode(CursorMode mode)
{
    initialize();

    if(mode == CursorMode::Normal)
    {
        glfwSetInputMode(_windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if(mode == CursorMode::Hidden)
    {
        glfwSetInputMode(_windowPtr, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else if(mode == CursorMode::Disable)
    {
        glfwSetInputMode(_windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        throw std::runtime_error("Unknown cursor mode.");
    }
    _cursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    initialize();

    return _fboAspectRatio;
}

void paz::resize_targets()
{
    for(auto n : initialize().renderTargets)
    {
        reinterpret_cast<Texture::Data*>(n)->resize(Window::ViewportWidth(),
            Window::ViewportHeight());
    }
}

bool paz::Window::Done()
{
    initialize();

    return glfwWindowShouldClose(_windowPtr);
}

void paz::Window::PollEvents()
{
    initialize();

    glfwPollEvents();
    GLFWgamepadstate state;
    if(glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
    {
        for(int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
        {
            const int idx = static_cast<int>(paz::convert_button(i));
            if(state.buttons[i] == GLFW_PRESS)
            {
                _gamepadActive = true;
                _mouseActive = false;
                if(!_gamepadDown[idx])
                {
                    _gamepadPressed[idx] = true;
                }
                _gamepadDown[idx] = true;
            }
            else if(state.buttons[i] == GLFW_RELEASE)
            {
                if(_gamepadDown[idx])
                {
                    _gamepadActive = true;
                    _mouseActive = false;
                    _gamepadReleased[idx] = true;
                }
                _gamepadDown[idx] = false;
            }
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.first = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.second = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.first = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.second = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftTrigger = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightTrigger = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }
    if(_cursorDisabled)
    {
        glfwSetCursorPos(_windowPtr, _windowWidth/2, _windowHeight/2);
    }
}

void paz::Window::EndFrame()
{
    initialize();

    _frameInProgress = false;

    static const unsigned int quadShaderId = []()
    {
        static const std::string headerStr = "#version " + std::to_string(paz::
            GlMajorVersion) + std::to_string(paz::GlMinorVersion) + "0 core\n";

        auto v = glCreateShader(GL_VERTEX_SHADER);
        {
            std::array<const char*, 2> srcStrs = {headerStr.c_str(),
                QuadVertSrc};
            glShaderSource(v, srcStrs.size(), srcStrs.data(), nullptr);
            glCompileShader(v);
            int success;
            glGetShaderiv(v, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                std::string errorLog = paz::get_log(v, false);
                throw std::runtime_error("Failed to compile final vertex functi"
                    "on:\n" + errorLog);
            }
        }

        auto f = glCreateShader(GL_FRAGMENT_SHADER);
        {
            std::array<const char*, 2> srcStrs = {headerStr.c_str(),
                QuadFragSrc};
            glShaderSource(f, srcStrs.size(), srcStrs.data(), nullptr);
            glCompileShader(f);
            int success;
            glGetShaderiv(f, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                std::string errorLog = paz::get_log(f, false);
                throw std::runtime_error("Failed to compile final fragment func"
                    "tion:\n" + errorLog);
            }
        }

        auto s = glCreateProgram();
        {
            glAttachShader(s, v);
            glAttachShader(s, f);
            glLinkProgram(s);
            GLint success;
            glGetProgramiv(s, GL_LINK_STATUS, &success);
            if(!success)
            {
                std::string errorLog = paz::get_log(s, true);
                throw std::runtime_error("Failed to link final shader program:"
                    "\n" + errorLog);
            }
        }

        return s;
    }();
    static const unsigned int quadBufId = []()
    {
        unsigned int a, b;
        glGenVertexArrays(1, &a);
        glBindVertexArray(a);
        glGenBuffers(1, &b);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, b);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*QuadPos.size(), QuadPos.
            data(), GL_STATIC_DRAW);
        return a;
    }();

    static const auto texLoc = glGetUniformLocation(quadShaderId, "tex");
    static const auto gammaLoc = glGetUniformLocation(quadShaderId, "gamma");
    static const auto ditherLoc = glGetUniformLocation(quadShaderId, "dither");

    glGetError();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    disable_blend_depth_cull();
    glUseProgram(quadShaderId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, final_framebuffer().colorAttachment(0)._data->
        _id);
    glUniform1i(texLoc, 0);
    glUniform1f(gammaLoc, _gamma);
    glUniform1f(ditherLoc, _dither ? 1.f : 0.f);
    glBindVertexArray(quadBufId);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, QuadPos.size()/2);
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error blitting to screen: " + gl_error(error)
            + ".");
    }

    glfwSwapBuffers(_windowPtr);
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        _frameStart).count()*1e-6;
    _frameStart = now;
}

void paz::Window::Quit()
{
    initialize();

    glfwSetWindowShouldClose(_windowPtr, GLFW_TRUE);
}

double paz::Window::FrameTime()
{
    initialize();

    return PrevFrameTime;
}

void paz::Window::SetMinSize(int width, int height)
{
    initialize();

    _minWidth = width;
    _minHeight = height;
    glfwSetWindowSizeLimits(_windowPtr, _minWidth, _minHeight, _maxWidth,
        _maxHeight);
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    _maxWidth = width;
    _maxHeight = height;
    glfwSetWindowSizeLimits(_windowPtr, _minWidth, _minHeight, _maxWidth,
        _maxHeight);
}

void paz::Window::MakeResizable()
{
    initialize();

    glfwSetWindowAttrib(_windowPtr, GLFW_RESIZABLE, GLFW_TRUE);
}

void paz::Window::MakeNotResizable()
{
    initialize();

    glfwSetWindowAttrib(_windowPtr, GLFW_RESIZABLE, GLFW_FALSE);
}

void paz::Window::Resize(int width, int height, bool viewportCoords)
{
    initialize();

    if(viewportCoords)
    {
        width = std::round(width/DpiScale());
        height = std::round(height/DpiScale());
    }
    else
    {
        if(_minWidth != GLFW_DONT_CARE)
        {
            width = std::max(width, _minWidth);
        }
        if(_maxWidth != GLFW_DONT_CARE)
        {
            width = std::min(width, _maxWidth);
        }
        if(_minHeight != GLFW_DONT_CARE)
        {
            height = std::max(height, _minHeight);
        }
        if(_maxHeight != GLFW_DONT_CARE)
        {
            height = std::min(height, _maxHeight);
        }
    }
    glfwSetWindowSize(_windowPtr, width, height);

    //TEMP - wait until resize has been completed
    if(viewportCoords)
    {
        while(ViewportWidth() != width || ViewportHeight() != height)
        {
            resize_callback(width, height);
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

paz::Image paz::Window::ReadPixels()
{
    initialize();

    if(_frameInProgress)
    {
        throw std::logic_error("Cannot read pixels before ending frame.");
    }

    const auto width = ViewportWidth();
    const auto height = ViewportHeight();

    std::vector<float> linear(4*width*height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, final_framebuffer().colorAttachment(0)._data->
        _id);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, linear.data());
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error reading window pixels: " + gl_error(
            error) + ".");
    }

    Image srgb(ImageFormat::RGBA8UNorm_sRGB, width, height);
    for(int i = 0; i < height; ++i)
    {
        for(int j = 0; j < width; ++j)
        {
            srgb.bytes()[4*(width*i + j) + 0] = to_srgb(linear[4*(width*i + j)
                + 0]);
            srgb.bytes()[4*(width*i + j) + 1] = to_srgb(linear[4*(width*i + j)
                + 1]);
            srgb.bytes()[4*(width*i + j) + 2] = to_srgb(linear[4*(width*i + j)
                + 2]);
            srgb.bytes()[4*(width*i + j) + 3] = linear[4*(width*i + j) + 3];
        }
    }

    return srgb;
}

void paz::begin_frame()
{
    _frameInProgress = true;
}

float paz::Window::DpiScale()
{
    initialize();

    return _hidpiEnabled ? static_cast<float>(_fboWidth)/_windowWidth : 1.f;
}

float paz::Window::UiScale()
{
    initialize();

    float xScale, yScale;
    glfwGetWindowContentScale(_windowPtr, &xScale, &yScale);
    if(!_hidpiEnabled)
    {
        xScale *= static_cast<float>(_windowWidth)/_fboWidth;
    }
    return xScale;
}

void paz::Window::DisableHidpi()
{
    initialize();

    _hidpiEnabled = false;
    resize_targets();
}

void paz::Window::EnableHidpi()
{
    initialize();

    _hidpiEnabled = true;
    resize_targets();
}

bool paz::Window::HidpiEnabled()
{
    initialize();

    return _hidpiEnabled;
}

bool paz::Window::HidpiSupported()
{
    initialize();

    return _fboWidth > _windowWidth;
}

void paz::Window::SetGamma(float gamma)
{
    initialize();

    _gamma = gamma;
}

void paz::Window::DisableDithering()
{
    initialize();

    _dither = false;
}

void paz::Window::EnableDithering()
{
    initialize();

    _dither = true;
}

int paz::Window::MaxAnisotropy()
{
    initialize();

    return _maxAnisotropy;
}

#endif
