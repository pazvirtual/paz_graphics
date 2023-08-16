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
static std::array<bool, paz::NumKeys> KeyDown;
static std::array<bool, paz::NumKeys> KeyPressed;
static std::array<bool, paz::NumKeys> KeyReleased;
static std::array<bool, paz::NumMouseButtons> MouseDown;
static std::array<bool, paz::NumMouseButtons> MousePressed;
static std::array<bool, paz::NumMouseButtons> MouseReleased;
static std::pair<double, double> MousePos;
static std::pair<double, double> ScrollOffset;
static std::array<bool, paz::NumGamepadButtons> GamepadDown;
static std::array<bool, paz::NumGamepadButtons> GamepadPressed;
static std::array<bool, paz::NumGamepadButtons> GamepadReleased;
static std::pair<double, double> GamepadLeftStick;
static std::pair<double, double> GamepadRightStick;
static double GamepadLeftTrigger = -1.;
static double GamepadRightTrigger = -1.;
static bool GamepadActive;
static bool MouseActive;
static bool CursorDisabled;
static bool FrameInProgress;
static bool HidpiEnabled = true;
static float Gamma = 2.2;
static bool Dither = false;
static const unsigned int QuadShaderId = []()
{
    paz::initialize();

    static const std::string headerStr = "#version " + std::to_string(paz::
        GlMajorVersion) + std::to_string(paz::GlMinorVersion) + "0 core\n";

    auto v = glCreateShader(GL_VERTEX_SHADER);
    {
        std::array<const char*, 2> srcStrs = {headerStr.c_str(), QuadVertSrc};
        glShaderSource(v, srcStrs.size(), srcStrs.data(), nullptr);
        glCompileShader(v);
        int success;
        glGetShaderiv(v, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            std::string errorLog = paz::get_log(v, false);
            throw std::runtime_error("Failed to compile final vertex function:"
                "\n" + errorLog);
        }
    }

    auto f = glCreateShader(GL_FRAGMENT_SHADER);
    {
        std::array<const char*, 2> srcStrs = {headerStr.c_str(), QuadFragSrc};
        glShaderSource(f, srcStrs.size(), srcStrs.data(), nullptr);
        glCompileShader(f);
        int success;
        glGetShaderiv(f, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            std::string errorLog = paz::get_log(f, false);
            throw std::runtime_error("Failed to compile final fragment function"
                ":\n" + errorLog);
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
            throw std::runtime_error("Failed to link final shader program:\n" +
                errorLog);
        }
    }

    return s;
}();
static const unsigned int QuadBufId = []()
{
    paz::initialize();

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

paz::Initializer& paz::initialize()
{
    static paz::Initializer initializer;
    return initializer;
}

static std::chrono::time_point<std::chrono::steady_clock> FrameStart = std::
    chrono::steady_clock::now();
static double PrevFrameTime = 1./60.;

static void key_callback(int key, int action)
{
    GamepadActive = false;
    MouseActive = false;

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
    GamepadActive = false;
    MouseActive = true;

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
    GamepadActive = false;
    MouseActive = true;

    MousePos.first = xPos;
    MousePos.second = WindowHeight - yPos;
}

static void scroll_callback(double xOffset, double yOffset)
{
    GamepadActive = false;
    MouseActive = true;

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
    ::GamepadPressed = {};
    ::GamepadReleased = {};
    ::GamepadLeftStick = {};
    ::GamepadRightStick = {};
    ::GamepadLeftTrigger = -1.;
    ::GamepadRightTrigger = -1.;
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
    WindowWidth = 0.5*displayWidth;
    WindowHeight = 0.5*displayHeight;

    // Create window and set as current context.
    WindowPtr = glfwCreateWindow(WindowWidth, WindowHeight,
        "PAZ_Graphics Window", nullptr, nullptr);
    WindowIsFullscreen = false;
    if(!WindowPtr)
    {
        throw std::runtime_error("Failed to create GLFW window. Your GPU may no"
            "t be OpenGL " + std::to_string(paz::GlMajorVersion) + "." + std::
            to_string(paz::GlMinorVersion) + " compatible.");
    }
    glfwMakeContextCurrent(WindowPtr);
    glfwGetWindowPos(WindowPtr, &PrevX, &PrevY);

    // Get window size in pixels.
    glfwGetFramebufferSize(WindowPtr, &FboWidth, &FboHeight);
    FboAspectRatio = static_cast<float>(FboWidth)/FboHeight;

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
            videoMode->height, videoMode->refreshRate);
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
    initialize();

    return WindowIsKey;
}

bool paz::Window::IsFullscreen()
{
    initialize();

    return WindowIsFullscreen;
}

int paz::Window::ViewportWidth()
{
    initialize();

    return ::HidpiEnabled ? FboWidth : WindowWidth;
}

int paz::Window::ViewportHeight()
{
    initialize();

    return ::HidpiEnabled ? FboHeight : WindowHeight;
}

int paz::Window::Width()
{
    initialize();

    return WindowWidth;
}

int paz::Window::Height()
{
    initialize();

    return WindowHeight;
}

bool paz::Window::KeyDown(Key key)
{
    initialize();

    return ::KeyDown.at(static_cast<int>(key));
}

bool paz::Window::KeyPressed(Key key)
{
    initialize();

    return ::KeyPressed.at(static_cast<int>(key));
}

bool paz::Window::KeyReleased(Key key)
{
    initialize();

    return ::KeyReleased.at(static_cast<int>(key));
}

bool paz::Window::MouseDown(int button)
{
    initialize();

    return ::MouseDown.at(button);
}

bool paz::Window::MousePressed(int button)
{
    initialize();

    return ::MousePressed.at(button);
}

bool paz::Window::MouseReleased(int button)
{
    initialize();

    return ::MouseReleased.at(button);
}

std::pair<double, double> paz::Window::MousePos()
{
    initialize();

    return ::MousePos;
}

std::pair<double, double> paz::Window::ScrollOffset()
{
    initialize();

    return ::ScrollOffset;
}

bool paz::Window::GamepadDown(GamepadButton button)
{
    initialize();

    return ::GamepadDown.at(static_cast<int>(button));
}

bool paz::Window::GamepadPressed(GamepadButton button)
{
    initialize();

    return ::GamepadPressed.at(static_cast<int>(button));
}

bool paz::Window::GamepadReleased(GamepadButton button)
{
    initialize();

    return ::GamepadReleased.at(static_cast<int>(button));
}

std::pair<double, double> paz::Window::GamepadLeftStick()
{
    initialize();

    return ::GamepadLeftStick;
}

std::pair<double, double> paz::Window::GamepadRightStick()
{
    initialize();

    return ::GamepadRightStick;
}

double paz::Window::GamepadLeftTrigger()
{
    initialize();

    return ::GamepadLeftTrigger;
}

double paz::Window::GamepadRightTrigger()
{
    initialize();

    return ::GamepadRightTrigger;
}

bool paz::Window::GamepadActive()
{
    initialize();

    return ::GamepadActive;
}

bool paz::Window::MouseActive()
{
    initialize();

    return ::MouseActive;
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
    initialize();

    return FboAspectRatio;
}

void paz::resize_targets()
{
    for(auto n : initialize()._renderTargets)
    {
        reinterpret_cast<Texture::Data*>(n)->resize(Window::ViewportWidth(),
            Window::ViewportHeight());
    }
}

bool paz::Window::Done()
{
    initialize();

    return glfwWindowShouldClose(WindowPtr);
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
                ::GamepadActive = true;
                ::MouseActive = false;
                if(!::GamepadDown[idx])
                {
                    ::GamepadPressed[idx] = true;
                }
                ::GamepadDown[idx] = true;
            }
            else if(state.buttons[i] == GLFW_RELEASE)
            {
                if(::GamepadDown[idx])
                {
                    ::GamepadActive = true;
                    ::MouseActive = false;
                    ::GamepadReleased[idx] = true;
                }
                ::GamepadDown[idx] = false;
            }
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftStick.first = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftStick.second = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightStick.first = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightStick.second = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > -0.9)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftTrigger = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > -0.9)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightTrigger = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }
    if(CursorDisabled)
    {
        glfwSetCursorPos(WindowPtr, 0, WindowHeight);
    }
}

void paz::Window::EndFrame()
{
    initialize();

    FrameInProgress = false;

    static const auto texLoc = glGetUniformLocation(QuadShaderId, "tex");
    static const auto gammaLoc = glGetUniformLocation(QuadShaderId, "gamma");
    static const auto ditherLoc = glGetUniformLocation(QuadShaderId, "dither");

    glGetError();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    disable_blend_depth_cull();
    glUseProgram(QuadShaderId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, final_framebuffer().colorAttachment(0)._data->
        _id);
    glUniform1i(texLoc, 0);
    glUniform1f(gammaLoc, Gamma);
    glUniform1f(ditherLoc, Dither ? 1.f : 0.f);
    glBindVertexArray(QuadBufId);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, QuadPos.size()/2);
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error blitting to screen: " + gl_error(error)
            + ".");
    }

    glfwSwapBuffers(WindowPtr);
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
}

void paz::Window::Quit()
{
    initialize();

    glfwSetWindowShouldClose(WindowPtr, GLFW_TRUE);
}

double paz::Window::FrameTime()
{
    initialize();

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
    }
    glfwSetWindowSize(WindowPtr, width, height);

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

paz::Image paz::Window::ReadPixels()
{
    initialize();

    if(FrameInProgress)
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
    FrameInProgress = true;
}

float paz::Window::DpiScale()
{
    initialize();

    return ::HidpiEnabled ? static_cast<float>(FboWidth)/WindowWidth : 1.f;
}

float paz::Window::UiScale()
{
    initialize();

    float xScale, yScale;
    glfwGetWindowContentScale(WindowPtr, &xScale, &yScale);
    if(!::HidpiEnabled)
    {
        xScale *= static_cast<float>(WindowWidth)/FboWidth;
    }
    return xScale;
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
    return FboWidth > WindowWidth;
}

void paz::Window::SetGamma(float gamma)
{
    Gamma = gamma;
}

void paz::Window::DisableDithering()
{
    Dither = false;
}

void paz::Window::EnableDithering()
{
    Dither = true;
}

#endif
