#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include "render_pass.hpp"
#include "keycodes.hpp"
#include "util_opengl.hpp"
#include "window.hpp"
#include "internal_data.hpp"
#ifdef PAZ_LINUX
#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3dcompiler.h>
#endif
#include <cmath>
#include <chrono>

#ifdef PAZ_LINUX
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
layout(location = 0) out vec4 color;
void main()
{
    color = texture(tex, uv);
    color.rgb = pow(color.rgb, vec3(1./gamma));
}
)===";
#else
static const std::string QuadVertSrc = 1 + R"===(
struct InputData
{
    float2 pos : ATTR0;
};
struct OutputData
{
    float4 glPosition : SV_Position;
    float2 uv : TEXCOORD0;
};
OutputData main(InputData input)
{
    OutputData output;
    output.glPosition = float4(input.pos, 0., 1.);
    output.uv = 0.5*input.pos + 0.5;
    return output;
}
)===";
static const std::string QuadFragSrc = 1 + R"===(
uniform Texture2D tex;
uniform SamplerState texSampler;
uniform float gamma;
struct InputData
{
    float4 glPosition : SV_Position; //TEMP - bad if necessary !
    float2 uv : TEXCOORD0;
};
struct OutputData
{
    float4 color : SV_TARGET0;
};
OutputData main(InputData input)
{
    OutputData output;
    output.color = tex.Sample(texSampler, float2(input.uv.x, 1. - input.uv.y));
    output.color.rgb = pow(output.color.rgb, (float3)(1./gamma));
    return output;
}
)===";
#endif

static constexpr std::array<float, 8> QuadPos =
{
     1, -1,
     1,  1,
    -1, -1,
    -1,  1
};

static GLFWwindow* WindowPtr;
#ifdef PAZ_WINDOWS
ID3D11Device* Device;
ID3D11DeviceContext* DeviceContext;
IDXGISwapChain* SwapChain;
ID3D11RenderTargetView* RenderTargetView;
#endif
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
static bool CursorDisabled;
static bool FrameInProgress;
static bool HidpiEnabled = true;
static float Gamma = 2.2;
#ifdef PAZ_LINUX
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
#else
static ID3DBlob* QuadVertBytecode = []()
{
    paz::initialize();

    ID3DBlob* res;
    ID3DBlob* error;
    const auto hr = D3DCompile(QuadVertSrc.c_str(), QuadVertSrc.size(), nullptr,
        nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &res, &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile final vertex shader: " +
            std::string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    return res;
}();
static ID3D11VertexShader* QuadVertShader = []()
{
    paz::initialize();

    ID3D11VertexShader* res;
    const auto hr = Device->CreateVertexShader(QuadVertBytecode->
        GetBufferPointer(), QuadVertBytecode->GetBufferSize(), nullptr, &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final vertex shader (HRESULT"
            " " + std::to_string(hr) + ").");
    }
    return res;
}();
static ID3D11PixelShader* QuadFragShader = []()
{
    paz::initialize();

    ID3DBlob* fragBlob;
    ID3DBlob* error;
    auto hr = D3DCompile(QuadFragSrc.c_str(), QuadFragSrc.size(), nullptr,
        nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &fragBlob, &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile final fragment shader: " +
            std::string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    ID3D11PixelShader* res;
    hr = Device->CreatePixelShader(fragBlob->GetBufferPointer(), fragBlob->
        GetBufferSize(), nullptr, &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final fragment shader (HRESU"
            "LT " + std::to_string(hr) + ").");
    }
    return res;
}();
static ID3D11Buffer* QuadBuf = []()
{
    paz::initialize();

    D3D11_BUFFER_DESC bufDescriptor = {};
    bufDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
    bufDescriptor.ByteWidth = sizeof(float)*QuadPos.size();
    bufDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = QuadPos.data();
    ID3D11Buffer* res;
    const auto hr = Device->CreateBuffer(&bufDescriptor, &data, &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final vertex buffer (HRESULT"
            " " + std::to_string(hr) + ").");
    }
    return res;
}();
static ID3D11InputLayout* QuadLayout = []()
{
    paz::initialize();

    D3D11_INPUT_ELEMENT_DESC inputElemDescriptor = {"ATTR", 0,
        DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
        D3D11_INPUT_PER_VERTEX_DATA, 0};
    ID3D11InputLayout* res;
    const auto hr = Device->CreateInputLayout(&inputElemDescriptor, 1,
        QuadVertBytecode->GetBufferPointer(), QuadVertBytecode->GetBufferSize(),
        &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final input layout (HRESULT "
            + std::to_string(hr) + ").");
    }
    return res;
}();
static ID3D11RasterizerState* BlitState = []()
{
    paz::initialize();

    D3D11_RASTERIZER_DESC rasterDescriptor = {};
    rasterDescriptor.FillMode = D3D11_FILL_SOLID;
    rasterDescriptor.CullMode = D3D11_CULL_NONE;
    rasterDescriptor.FrontCounterClockwise = true;
    rasterDescriptor.DepthClipEnable = true;
    ID3D11RasterizerState* res;
    const auto hr = Device->CreateRasterizerState(&rasterDescriptor, &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final rasterizer state (HRES"
            "ULT " + std::to_string(hr) + ").");
    }
    return res;
}();
static ID3D11Buffer* GammaBuf = []()
{
    D3D11_BUFFER_DESC bufDescriptor = {};
    bufDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    bufDescriptor.ByteWidth = 16;
    bufDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* res;
    const auto hr = Device->CreateBuffer(&bufDescriptor, nullptr, &res);
    if(hr)
    {
        throw std::runtime_error("Failed to create final constant buffer (HRESU"
            "LT " + std::to_string(hr) + ").");
    }
    return res;
}();
#endif

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

    MousePos.first = xPos;
    MousePos.second = WindowHeight - yPos;
}

static void scroll_callback(double xOffset, double yOffset)
{
    GamepadActive = false;

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

static void poll_events()
{
    glfwPollEvents();
    GLFWgamepadstate state;
    if(glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
    {
        for(int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i)
        {
            const int idx = static_cast<int>(paz::convert_button(i));
            if(state.buttons[i] == GLFW_PRESS)
            {
                GamepadActive = true;
                if(!GamepadDown[idx])
                {
                    GamepadPressed[idx] = true;
                }
                GamepadDown[idx] = true;
            }
            else if(state.buttons[i] == GLFW_RELEASE)
            {
                if(GamepadDown[idx])
                {
                    GamepadActive = true;
                    GamepadReleased[idx] = true;
                }
                GamepadDown[idx] = false;
            }
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > 0.1)
        {
            GamepadActive = true;
            GamepadLeftStick.first = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > 0.1)
        {
            GamepadActive = true;
            GamepadLeftStick.second = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]) > 0.1)
        {
            GamepadActive = true;
            GamepadRightStick.first = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        }
        if(std::abs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > 0.1)
        {
            GamepadActive = true;
            GamepadRightStick.second = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > -0.9)
        {
            GamepadActive = true;
            GamepadLeftTrigger = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
        }
        if(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > -0.9)
        {
            GamepadActive = true;
            GamepadRightTrigger = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
        }
    }
    if(CursorDisabled)
    {
        glfwSetCursorPos(WindowPtr, 0, WindowHeight);
    }
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
            "ng a remote shell"
#ifdef PAZ_LINUX
            " and need to set the `DISPLAY` environment variable.");
#else
            ".");
#endif
    }

    // Set context hints.
#ifdef PAZ_LINUX
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GlMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GlMinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Get display size.
    auto* monitor = glfwGetPrimaryMonitor();
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
#ifdef PAZ_LINUX
    if(!WindowPtr)
    {
        throw std::runtime_error("Failed to open GLFW window. Your GPU may not "
            "be OpenGL " + std::to_string(paz::GlMajorVersion) + "." + std::
            to_string(paz::GlMinorVersion) + " compatible.");
    }
    glfwMakeContextCurrent(WindowPtr);
#else
    if(!WindowPtr)
    {
        throw std::runtime_error("Failed to open GLFW window.");
    }
    DXGI_SWAP_CHAIN_DESC swapChainDescriptor = {};
    swapChainDescriptor.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDescriptor.BufferDesc.RefreshRate.Denominator = 0;
    swapChainDescriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 1; //TEMP ?
    swapChainDescriptor.OutputWindow = glfwGetWin32Window(WindowPtr);
    swapChainDescriptor.Windowed = true;
    static const D3D_FEATURE_LEVEL featureLvlReq = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL featureLvl;
    auto hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED, &featureLvlReq, 1,
        D3D11_SDK_VERSION, &swapChainDescriptor, &SwapChain, &Device,
        &featureLvl, &DeviceContext);
    if(hr)
    {
        throw std::runtime_error("Failed to intialize Direct3D (HRESULT " +
            std::to_string(hr) + ").");
    }
    if(featureLvl != featureLvlReq)
    {
        throw std::runtime_error("Failed to initialize Direct3D: Feature level "
            + std::to_string(featureLvlReq) + " not supported (got " + std::
            to_string(featureLvl) + ").");
    }
    ID3D11Texture2D* framebuffer;
    hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<
        void**>(&framebuffer));
    if(hr)
    {
        throw std::runtime_error("Failed to get default framebuffer.");
    }
    hr = Device->CreateRenderTargetView(framebuffer, nullptr,
        &RenderTargetView);
    if(hr)
    {
        throw std::runtime_error("Failed to create default framebuffer view.");
    }
    framebuffer->Release();
#endif
    glfwGetWindowPos(WindowPtr, &PrevX, &PrevY);

    // Get window size in pixels.
    glfwGetFramebufferSize(WindowPtr, &FboWidth, &FboHeight);
    FboAspectRatio = static_cast<float>(FboWidth)/FboHeight;

#ifdef PAZ_LINUX
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
#endif

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

    // Poll events.
    poll_events();
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

#ifdef PAZ_LINUX
        // Keep vsync.
        glfwSwapInterval(1);
#endif
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

#ifdef PAZ_LINUX
        // Keep vsync.
        glfwSwapInterval(1);
#endif
    }
}

#ifdef PAZ_WINDOWS
ID3D11Device* paz::d3d_device()
{
    return Device;
}
ID3D11DeviceContext* paz::d3d_context()
{
    return DeviceContext;
}
#endif

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

    return HidpiEnabled ? FboWidth : WindowWidth;
}

int paz::Window::ViewportHeight()
{
    initialize();

    return HidpiEnabled ? FboHeight : WindowHeight;
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
#ifdef PAZ_WINDOWS
    // Release resources.
    RenderTargetView->Release();

    // Resize back buffer.
    auto hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if(hr)
    {
        throw std::runtime_error("Failed to resize back buffer (HRESULT " +
            std::to_string(hr) + ").");
    }

    // Regenerate render target view.
    ID3D11Texture2D* framebuffer;
    hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<
        void**>(&framebuffer));
    if(hr)
    {
        throw std::runtime_error("Failed to get default framebuffer.");
    }
    hr = Device->CreateRenderTargetView(framebuffer, nullptr,
        &RenderTargetView);
    if(hr)
    {
        throw std::runtime_error("Failed to create default framebuffer view.");
    }
    framebuffer->Release();
#endif

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

void paz::Window::EndFrame()
{
    initialize();

    FrameInProgress = false;

#ifdef PAZ_LINUX
    static const auto texLoc = glGetUniformLocation(QuadShaderId, "tex");
    static const auto gammaLoc = glGetUniformLocation(QuadShaderId, "gamma");

    glGetError();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    disable_blend_depth_cull();
    glUseProgram(QuadShaderId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, final_framebuffer().colorAttachment(0)._data->
        _id);
    glUniform1i(texLoc, 0);
    glUniform1f(gammaLoc, Gamma);
    glBindVertexArray(QuadBufId);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, QuadPos.size()/2);
    const GLenum error = glGetError();
    if(error != GL_NO_ERROR)
    {
        throw std::runtime_error("Error blitting to screen: " + gl_error(error)
            + ".");
    }

    glfwSwapBuffers(WindowPtr);
#else
    DeviceContext->RSSetState(BlitState);
    D3D11_VIEWPORT viewport = {};
    viewport.Width = ViewportWidth();
    viewport.Height = ViewportHeight();
    DeviceContext->RSSetViewports(1, &viewport);
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);
    DeviceContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    DeviceContext->IASetInputLayout(QuadLayout);
    const unsigned int stride = 2*sizeof(float);
    const unsigned int offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &QuadBuf, &stride, &offset);
    DeviceContext->VSSetShader(QuadVertShader, nullptr, 0);
    DeviceContext->PSSetShader(QuadFragShader, nullptr, 0);
    DeviceContext->PSSetShaderResources(0, 1, &final_framebuffer().
        colorAttachment(0)._data->_resourceView);
    DeviceContext->PSSetSamplers(0, 1, &final_framebuffer().colorAttachment(0).
        _data->_sampler);
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = DeviceContext->Map(GammaBuf, 0, D3D11_MAP_WRITE_DISCARD, 0,
        &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map final fragment function constan"
            "t buffer (HRESULT " + std::to_string(hr) + ").");
    }
    std::copy(&Gamma, &Gamma + 1, reinterpret_cast<float*>(mappedSr.pData));
    DeviceContext->Unmap(GammaBuf, 0);
    DeviceContext->PSSetConstantBuffers(0, 1, &GammaBuf);
    DeviceContext->Draw(QuadPos.size()/2, 0);

    SwapChain->Present(1, 0);
#endif
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        FrameStart).count()*1e-6;
    FrameStart = now;
    poll_events();
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

#ifdef PAZ_LINUX
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
    for(int i = 0; i < height(); ++i)
    {
        for(int j = 0; j < width(); ++j)
        {
            srgb.bytes()[4*(width()*i + j) + 0] = to_srgb(linear[4*(width*i + j)
                + 0]);
            srgb.bytes()[4*(width()*i + j) + 1] = to_srgb(linear[4*(width*i + j)
                + 1]);
            srgb.bytes()[4*(width()*i + j) + 2] = to_srgb(linear[4*(width*i + j)
                + 2]);
            srgb.bytes()[4*(width()*i + j) + 3] = linear[4*(width*i + j) + 3];
        }
    }
#else
    D3D11_TEXTURE2D_DESC descriptor = {};
    descriptor.Width = width;
    descriptor.Height = height;
    descriptor.MipLevels = 1;
    descriptor.ArraySize = 1;
    descriptor.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
    descriptor.SampleDesc.Count = 1;
    descriptor.Usage = D3D11_USAGE_STAGING;
    descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging;
    auto hr = d3d_device()->CreateTexture2D(&descriptor, nullptr, &staging);
    if(hr)
    {
        throw std::runtime_error("Failed to create staging texture (HRESULT " +
            std::to_string(hr) + ").");
    }

    DeviceContext->CopySubresourceRegion(staging, 0, 0, 0, 0,
        final_framebuffer().colorAttachment(0)._data->_texture, 0, nullptr);

    D3D11_MAPPED_SUBRESOURCE mappedSr;
    hr = DeviceContext->Map(staging, 0, D3D11_MAP_READ, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map staging texture (HRESULT " +
            std::to_string(hr) + ").");
    }

    Image srgb(ImageFormat::RGBA8UNorm_sRGB, width, height);
    static constexpr double d = 1./std::numeric_limits<std::uint16_t>::max();
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            const int yFlipped = height - 1 - y;
            std::uint16_t* rowStart = reinterpret_cast<std::uint16_t*>(
                reinterpret_cast<unsigned char*>(mappedSr.pData) + mappedSr.
                RowPitch*yFlipped);
            for(int i = 0; i < 3; ++i)
            {
                srgb.bytes()[4*(width*y + x) + i] = to_srgb(*(rowStart + 4*x +
                    i)*d);
            }
            srgb.bytes()[4*(width*y + x) + 3] = *(rowStart + 4*x + 3)*d;
        }
    }

    DeviceContext->Unmap(staging, 0);
#endif

    return srgb;
}

void paz::begin_frame()
{
    FrameInProgress = true;
}

float paz::Window::DpiScale()
{
    initialize();

    return HidpiEnabled ? static_cast<float>(FboWidth)/WindowWidth : 1.f;
}

float paz::Window::UiScale()
{
    initialize();

    float xScale, yScale;
    glfwGetWindowContentScale(WindowPtr, &xScale, &yScale);
    if(!HidpiEnabled)
    {
        xScale *= static_cast<float>(WindowWidth)/FboWidth;
    }
    return xScale;
}

void paz::Window::DisableHidpi()
{
    initialize();

    HidpiEnabled = false;
    resize_targets();
}

void paz::Window::EnableHidpi()
{
    initialize();

    HidpiEnabled = true;
    resize_targets();
}

void paz::Window::SetGamma(float gamma)
{
    Gamma = gamma;
}

#endif
