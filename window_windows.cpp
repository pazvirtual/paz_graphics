#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "render_pass.hpp"
#include "keycodes.hpp"
#include "common.hpp"
#include "internal_data.hpp"
#include "util_windows.hpp"
#include "gamepad.hpp"
#include "windows.hpp"
#include <cmath>
#include <chrono>
#include <algorithm>

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
uniform float2 gammaDither;
uint hash(in uint x)
{
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
uint hash(in uint2 v)
{
    return hash(v.x^hash(v.y));
}
float construct_float(in uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    float f = asfloat(m);
    return f - 1.;
}
float unif_rand(in float2 v)
{
    return construct_float(hash(asuint(v)));
}
struct InputData
{
    float4 glPosition : SV_Position;
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
    output.color.rgb = pow(output.color.rgb, (float3)(1./gammaDither.x));
    output.color.rgb += gammaDither.y*lerp(-0.5/255., 0.5/255., unif_rand(input.
        uv));
    return output;
}
)===";

static constexpr std::array<float, 8> QuadPos =
{
     1, -1,
     1,  1,
    -1, -1,
    -1,  1
};

static HWND WindowHandle;
static HMONITOR MonitorHandle;
static ID3D11Device* Device;
static ID3D11DeviceContext* DeviceContext;
static IDXGISwapChain* SwapChain;
static ID3D11RenderTargetView* RenderTargetView;
static int WindowWidth;
static int WindowHeight;
static bool WindowIsKey;
static bool FrameAction;
static bool CursorTracked;
static int PrevX;
static int PrevY;
static int PrevHeight;
static int PrevWidth;
static int FboWidth;
static int FboHeight;
static float FboAspectRatio;
static int MinWidth = 0;
static int MinHeight = 0;
static int MaxWidth = 0;
static int MaxHeight = 0;
static bool Resizable = true;
static bool Done;
static paz::CursorMode CurCursorMode = paz::CursorMode::Normal;
static std::array<bool, paz::NumKeys> KeyDown;
static std::array<bool, paz::NumKeys> KeyPressed;
static std::array<bool, paz::NumKeys> KeyReleased;
static std::array<bool, paz::NumMouseButtons> MouseDown;
static std::array<bool, paz::NumMouseButtons> MousePressed;
static std::array<bool, paz::NumMouseButtons> MouseReleased;
static std::pair<double, double> MousePos;
static std::pair<double, double> PrevMousePos;
static std::pair<double, double> VirtMousePos;
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
POINT PriorCursorPos;
static bool FrameInProgress;
static bool HidpiEnabled = true;
static float Gamma = 2.2;
static bool Dither = false;

static DWORD window_style()
{
    DWORD style = WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
    if(MonitorHandle)
    {
        style |= WS_POPUP;
    }
    else
    {
        style |= WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION;
        if(Resizable)
        {
            style |= WS_MAXIMIZEBOX|WS_THICKFRAME;
        }
    }
    return style;
}

static DWORD window_ex_style()
{
    DWORD exStyle = WS_EX_APPWINDOW;
    return exStyle;
}

static void update_styles()
{
    DWORD style = GetWindowLong(WindowHandle, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW|WS_POPUP);
    style |= window_style();

    RECT rect;
    GetClientRect(WindowHandle, &rect);

    AdjustWindowRectEx(&rect, style, FALSE, window_ex_style());

    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(&rect.left));
    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(&rect.right));
    SetWindowLong(WindowHandle, GWL_STYLE, style);
    SetWindowPos(WindowHandle, HWND_TOP, rect.left, rect.top, rect.right - rect.
        left, rect.bottom - rect.top, SWP_FRAMECHANGED|SWP_NOACTIVATE|
        SWP_NOZORDER);
}

static bool poll_gamepad_state(paz::GamepadState& state)
{
    return false; //TEMP
}

paz::Initializer& paz::initialize()
{
    static paz::Initializer initializer;
    return initializer;
}

static double PrevFrameTime = 1./60.;

static void center_cursor()
{
    POINT pos = {WindowWidth/2, WindowHeight/2};
    ClientToScreen(WindowHandle, &pos);
    SetCursorPos(pos.x, pos.y);
}

static void capture_cursor()
{
    RECT clipRect;
    GetClientRect(WindowHandle, &clipRect);
    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(&clipRect.left));
    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(&clipRect.right));
    ClipCursor(&clipRect);
}

static void release_cursor()
{
    ClipCursor(nullptr);
}

static void enable_raw_mouse_motion()
{
    const RAWINPUTDEVICE rid = {0x01, 0x02, 0, WindowHandle};
    if(!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        throw std::runtime_error("Failed to register raw input device");
    }
}

static void disable_raw_mouse_motion()
{
    const RAWINPUTDEVICE rid = {0x01, 0x02, RIDEV_REMOVE, nullptr};
    if(!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        throw std::runtime_error("Failed to remove raw input device");
    }
}

static void disable_cursor()
{
    GetCursorPos(&PriorCursorPos);
    ScreenToClient(WindowHandle, &PriorCursorPos);
    center_cursor();
    SetCursor(nullptr);
    capture_cursor();
    enable_raw_mouse_motion();
}

static void enable_cursor()
{
    disable_raw_mouse_motion();
    release_cursor();
    PrevMousePos.first = PriorCursorPos.x;
    PrevMousePos.second = WindowHeight - PriorCursorPos.y;
    ClientToScreen(WindowHandle, &PriorCursorPos);
    SetCursorPos(PriorCursorPos.x, PriorCursorPos.y);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

static void acquire_monitor()
{
    if(!MonitorHandle)
    {
        SetThreadExecutionState(ES_CONTINUOUS|ES_DISPLAY_REQUIRED);
        MonitorHandle = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
    }
}

static void release_monitor()
{
    if(MonitorHandle)
    {
        SetThreadExecutionState(ES_CONTINUOUS);
        MonitorHandle = nullptr;
    }
}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM
    lParam)
{
    if(!WindowHandle)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    switch(uMsg)
    {
        case WM_MOUSEACTIVATE:
        {
            // Postpone disabling cursor when the window was activated by
            // clicking a caption button.
            if(HIWORD(lParam) == WM_LBUTTONDOWN)
            {
                if(LOWORD(lParam) != HTCLIENT)
                {
                    FrameAction = true;
                }
            }
            break;
        }
        case WM_CAPTURECHANGED:
        {
            // Disable the cursor once the caption button action has been
            // completed or canceled.
            if(!lParam && FrameAction)
            {
                if(CursorDisabled)
                {
                    disable_cursor();
                }
                FrameAction = false;
            }
            break;
        }
        case WM_SETFOCUS:
        {
            WindowIsKey = true;
            // Do not disable cursor while the user is interacting with a
            // caption button.
            if(FrameAction)
            {
                break;
            }
            if(CursorDisabled)
            {
                disable_cursor();
            }
            return 0;
        }
        case WM_KILLFOCUS:
        {
            WindowIsKey = false;
            if(CursorDisabled)
            {
                enable_cursor();
            }
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            switch(wParam&0xfff0)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                {
                    if(MonitorHandle)
                    {
                        return 0;
                    }
                    else
                    {
                        break;
                    }
                }
                case SC_KEYMENU:
                {
                    return 0;
                }
            }
            break;
        }
        case WM_CLOSE:
        {
            ::Done = true;
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const bool released = HIWORD(lParam)&KF_UP; // Otherwise, pressed

            int key = HIWORD(lParam)&(KF_EXTENDED|0xff);
            if(!key)
            {
                // NOTE: Some synthetic key messages have a key of zero
                // HACK: Map the virtual key back to a usable key
                key = MapVirtualKey(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC);
            }

            // HACK: Alt+PrtSc has a different key than just PrtSc
            if(key == 0x54)
            {
                key = 0x137;
            }
            // HACK: Ctrl+Pause has a different key than just Pause
            else if(key == 0x146)
            {
                key = 0x45;
            }
            // HACK: CJK IME sets the extended bit for right Shift
            else if(key == 0x136)
            {
                key = 0x36;
            }

            paz::Key k = paz::convert_keycode(key);

            // The Ctrl keys require special handling
            if(wParam == VK_CONTROL)
            {
                if(HIWORD(lParam)&KF_EXTENDED)
                {
                    // Right side keys have the extended key bit set
                    k = paz::Key::RightControl;
                }
                else
                {
                    // NOTE: Alt Gr sends Left Ctrl followed by Right Alt
                    // HACK: We only want one event for Alt Gr, so if we detect
                    //       this sequence we discard this Left Ctrl message now
                    //       and later report Right Alt normally
                    MSG next;
                    const DWORD time = GetMessageTime();

                    if(PeekMessage(&next, nullptr, 0, 0, PM_NOREMOVE))
                    {
                        if(next.message == WM_KEYDOWN || next.message ==
                            WM_SYSKEYDOWN || next.message == WM_KEYUP || next.
                            message == WM_SYSKEYUP)
                        {
                            if(next.wParam == VK_MENU && (HIWORD(next.lParam)&
                                KF_EXTENDED) && next.time == time)
                            {
                                // Next message is Right Alt down so discard this
                                break;
                            }
                        }
                    }

                    // This is a regular Left Ctrl message
                    k = paz::Key::LeftControl;
                }
            }
            else if(wParam == VK_PROCESSKEY)
            {
                // IME notifies that keys have been filtered by setting the
                // virtual key-code to VK_PROCESSKEY
                break;
            }

            if(released && wParam == VK_SHIFT)
            {
                GamepadActive = false;
                MouseActive = false;
                // HACK: Release both Shift keys on Shift up event, as when both
                //       are pressed the first release does not emit any event
                // NOTE: The other half of this is in _glfwPollEventsWin32
                static constexpr int l = static_cast<int>(paz::Key::LeftShift);
                static constexpr int r = static_cast<int>(paz::Key::RightShift);
                KeyDown[l] = false;
                KeyReleased[l] = true;
                KeyDown[r] = false;
                KeyReleased[r] = true;
            }
            else if(wParam == VK_SNAPSHOT)
            {
                GamepadActive = false;
                MouseActive = false;
                // HACK: Key down is not reported for the Print Screen key
                const int i = static_cast<int>(k);
                KeyDown[i] = false;
                KeyPressed[i] = true;
                KeyReleased[i] = true;
            }
            else if(k == paz::Key::Unknown)
            {
                break;
            }
            else
            {
                GamepadActive = false;
                MouseActive = false;
                const int i = static_cast<int>(k);
                if(released)
                {
                    KeyDown[i] = false;
                    KeyReleased[i] = true;
                }
                else
                {
                    KeyDown[i] = true;
                    KeyPressed[i] = true;
                }
            }

            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            GamepadActive = false;
            MouseActive = true;

            int button;
            if(uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
            {
                button = static_cast<int>(paz::MouseButton::Left);
            }
            else if(uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
            {
                button = static_cast<int>(paz::MouseButton::Right);
            }
            else if(uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
            {
                button = static_cast<int>(paz::MouseButton::Middle);
            }
            else if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
            {
                button = static_cast<int>(paz::MouseButton::Back);
            }
            else // XBUTTON2
            {
                button = static_cast<int>(paz::MouseButton::Forward);
            }

            if(std::none_of(MouseDown.begin(), MouseDown.end(), [](bool x){
                return x; }))
            {
                SetCapture(hWnd);
            }

            if(uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg ==
                WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN)
            {
                MouseDown[button] = true;
                MousePressed[button] = true;
            }
            else
            {
                MouseDown[button] = false;
                MouseReleased[button] = true;
            }

            if(std::none_of(MouseDown.begin(), MouseDown.end(), [](bool x){
                return x; }))
            {
                ReleaseCapture();
            }

            if(uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP)
            {
                return TRUE;
            }

            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if(!CursorTracked)
            {
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = WindowHandle;
                TrackMouseEvent(&tme);
                CursorTracked = true;
            }

            if(CursorDisabled)
            {
                break;
            }

            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            if(VirtMousePos.first != x || VirtMousePos.second != y)
            {
                GamepadActive = false;
                MouseActive = true;
                VirtMousePos.first = x;
                VirtMousePos.second = y;
                MousePos.first = VirtMousePos.first;
                MousePos.second = WindowHeight - VirtMousePos.second;
            }

            PrevMousePos.first = x;
            PrevMousePos.second = y;

            return 0;
        }
        case WM_INPUT:
        {
            if(!CursorDisabled)
            {
                break;
            }

            UINT size = 0;
            HRAWINPUT ri = reinterpret_cast<HRAWINPUT>(lParam);
            GetRawInputData(ri, RID_INPUT, nullptr, &size, sizeof(
                RAWINPUTHEADER));

            std::vector<unsigned char> buf(size);
            if(GetRawInputData(ri, RID_INPUT, buf.data(), &size, sizeof(
                RAWINPUTHEADER)) == std::numeric_limits<UINT>::max())
            {
                throw std::runtime_error("Failed to retrieve raw input data.");
            }

            const auto& data = reinterpret_cast<const RAWINPUT&>(*buf.data());
            int deltaX, deltaY;
            if(data.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE)
            {
                deltaX = data.data.mouse.lLastX - PrevMousePos.first;
                deltaY = data.data.mouse.lLastY - PrevMousePos.second;
            }
            else
            {
                deltaX = data.data.mouse.lLastX;
                deltaY = data.data.mouse.lLastY;
            }

            const double x = VirtMousePos.first + deltaX;
            const double y = VirtMousePos.second + deltaY;
            if(VirtMousePos.first != x || VirtMousePos.second != y)
            {
                GamepadActive = false;
                MouseActive = true;
                VirtMousePos.first = x;
                VirtMousePos.second = y;
                MousePos.first = VirtMousePos.first - WindowWidth/2;
                MousePos.second = WindowHeight/2 - VirtMousePos.second;
            }

            PrevMousePos.first += deltaX;
            PrevMousePos.second += deltaY;

            break;
        }
        case WM_MOUSELEAVE:
        {
            GamepadActive = false;
            MouseActive = true;
            CursorTracked = false;
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            GamepadActive = false;
            MouseActive = true;
            ScrollOffset.second = static_cast<double>(GET_WHEEL_DELTA_WPARAM(
                wParam))/WHEEL_DELTA;
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            GamepadActive = false;
            MouseActive = true;
            ScrollOffset.first = -static_cast<double>(GET_WHEEL_DELTA_WPARAM(
                wParam))/WHEEL_DELTA;
            return 0;
        }
        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP:
        {
            if(FrameAction)
            {
                break;
            }
            // Enable the cursor while the user is moving or resizing the window
            // or using the window menu.
            if(CursorDisabled)
            {
                enable_cursor();
            }
            break;
        }
        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP:
        {
            if(FrameAction)
            {
                break;
            }
            // Disable the cursor once the user is done moving or resizing the
            // window or using the menu.
            if(CursorDisabled)
            {
                disable_cursor();
            }
            break;
        }
        case WM_SIZE:
        {
            WindowWidth = LOWORD(lParam);
            WindowHeight = HIWORD(lParam);
            FboWidth = WindowWidth;
            FboHeight = WindowHeight;
            FboAspectRatio = static_cast<float>(FboWidth)/FboHeight;
            if(Device)
            {
                paz::resize_targets();
            }

//            const bool iconified = wParam == SIZE_MINIMIZED;
//            const bool maximized = wParam == SIZE_MAXIMIZED || (WindowIsMaximized && wParam != SIZE_RESTORED);
//
//            if (_glfw.win32.capturedCursorWindow == window)
//                captureCursor(window);
//
//            if (window->win32.iconified != iconified)
//                _glfwInputWindowIconify(window, iconified);
//
//            if (window->win32.maximized != maximized)
//                _glfwInputWindowMaximize(window, maximized);
//
//            if (width != window->win32.width || height != window->win32.height)
//            {
//                window->win32.width = width;
//                window->win32.height = height;
//
//                _glfwInputFramebufferSize(window, width, height);
//                _glfwInputWindowSize(window, width, height);
//            }
//
//            if(MonitorHandle && window->win32.iconified != iconified)
//            {
//                if(iconified)
//                {
//                    releaseMonitor(window);
//                }
//                else
//                {
//                    acquireMonitor(window);
//                    fitToMonitor(window);
//                }
//            }
//
//            window->win32.iconified = iconified;
//            window->win32.maximized = maximized;
            return 0;
        }
        case WM_MOVE:
        {
            if(!MonitorHandle)
            {
                PrevX = static_cast<int>(static_cast<short>(LOWORD(lParam)));
                PrevY = static_cast<int>(static_cast<short>(HIWORD(lParam)));
                if(CursorDisabled)
                {
                    RECT clipRect;
                    GetClientRect(WindowHandle, &clipRect);
                    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(
                        &clipRect.left));
                    ClientToScreen(WindowHandle, reinterpret_cast<POINT*>(
                        &clipRect.right));
                    ClipCursor(&clipRect);
                }
            }
            return 0;
        }
        case WM_GETMINMAXINFO:
        {
            RECT frame = {};
            MINMAXINFO& mmi = *reinterpret_cast<MINMAXINFO*>(lParam);
            if(MonitorHandle)
            {
                break;
            }

            if(paz::initialize().isWindows10Version1607OrGreater)
            {
                paz::initialize().adjustWindowRectExForDpi(&frame,
                    window_style(), FALSE, window_ex_style(), paz::initialize().
                    getDpiForWindow(WindowHandle));
            }
            else
            {
                AdjustWindowRectEx(&frame, window_style(), FALSE,
                    window_ex_style());
            }

            if(MinWidth && MinHeight)
            {
                mmi.ptMinTrackSize.x = MinWidth + frame.right - frame.left;
                mmi.ptMinTrackSize.y = MinHeight + frame.bottom - frame.top;
            }

            if(MaxWidth && MaxHeight)
            {
                mmi.ptMaxTrackSize.x = MaxWidth + frame.right - frame.left;
                mmi.ptMaxTrackSize.y = MaxHeight + frame.bottom - frame.top;
            }

            return 0;
        }
        case WM_ERASEBKGND:
        {
            return TRUE; //TEMP - verify
        }
        case WM_DWMCOMPOSITIONCHANGED:
        case WM_DWMCOLORIZATIONCOLORCHANGED:
        {
            return 0; //TEMP - verify
        }
        case WM_GETDPISCALEDSIZE:
        {
            // Adjust the window size to keep the content area size constant
            if (paz::initialize().isWindows10Version1703OrGreater)
            {
                RECT source = {}, target = {};
                SIZE& size = *reinterpret_cast<SIZE*>(lParam);

                paz::initialize().adjustWindowRectExForDpi(&source,
                    window_style(), FALSE, window_ex_style(), paz::initialize().
                    getDpiForWindow(WindowHandle));
                paz::initialize().adjustWindowRectExForDpi(&target,
                    window_style(), FALSE, window_ex_style(), LOWORD(wParam));

                size.cx += (target.right - target.left) - (source.right -
                    source.left);
                size.cy += (target.bottom - target.top) - (source.bottom -
                    source.top);
                return TRUE;
            }

            break;
        }
        case WM_DPICHANGED:
        {
            const float xscale = HIWORD(wParam)/static_cast<float>(USER_DEFAULT_SCREEN_DPI);
            const float yscale = LOWORD(wParam)/static_cast<float>(USER_DEFAULT_SCREEN_DPI);

            // Resize windowed mode windows that either permit rescaling or that
            // need it to compensate for non-client area scaling
            if (!MonitorHandle && paz::initialize().isWindows10Version1703OrGreater)
            {
                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(WindowHandle, HWND_TOP,
                             suggested->left,
                             suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOACTIVATE | SWP_NOZORDER);
            }

            if(Device)
            {
                paz::resize_targets();
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if(LOWORD(lParam) == HTCLIENT)
            {
                if(CurCursorMode == paz::CursorMode::Normal)
                {
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                }
                else
                {
                    SetCursor(nullptr);
                }
                return TRUE;
            }
            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

paz::Initializer::~Initializer() {}

paz::Initializer::Initializer()
{
    // Load functions from DLLs.
    const HMODULE user32 = LoadLibrary(L"user32.dll");
    getDpiForWindow = reinterpret_cast<UINT(*)(HWND)>(GetProcAddress(user32,
        "GetDpiForWindow"));
    adjustWindowRectExForDpi = reinterpret_cast<BOOL(*)(LPRECT, DWORD, BOOL,
        DWORD, UINT)>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
    setProcessDPIAware = reinterpret_cast<BOOL(*)(void)>(GetProcAddress(user32,
        "SetProcessDPIAware"));
    setProcessDpiAwarenessContext = reinterpret_cast<BOOL(*)(
        DPI_AWARENESS_CONTEXT)>(GetProcAddress(user32,
        "SetProcessDpiAwarenessContext"));
    const HMODULE shcore = LoadLibrary(L"shcore.dll");
    setProcessDpiAwareness = reinterpret_cast<HRESULT(*)(
        PROCESS_DPI_AWARENESS)>(GetProcAddress(shcore,
        "SetProcessDpiAwareness"));
    const HMODULE ntdll = LoadLibrary(L"ntdll.dll");
    rtlVerifyVersionInfo = reinterpret_cast<LONG(*)(OSVERSIONINFOEX*, ULONG,
        ULONGLONG)>(GetProcAddress(ntdll, "RtlVerifyVersionInfo"));

    // Check Windows version.
    OSVERSIONINFOEX osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.dwMajorVersion = 10;
    osvi.dwMinorVersion = 0;
    osvi.dwBuildNumber = 1703;
    ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION,
        VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    isWindows10Version1703OrGreater = !rtlVerifyVersionInfo(&osvi,
        VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER, cond);
    osvi.dwBuildNumber = 1607;
    isWindows10Version1607OrGreater = !rtlVerifyVersionInfo(&osvi,
        VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER, cond);
    isWindows8Point1OrGreater = true; //TEMP
    isWindowsVistaOrGreater = true; //TEMP

    // Set up DPI awareness.
    if(isWindows10Version1703OrGreater)
    {
        setProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
    else if(isWindows8Point1OrGreater)
    {
        setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    }
    else if(isWindowsVistaOrGreater)
    {
        setProcessDPIAware();
    }

    // Get display size. (Primary monitor starts at `(0, 0)`.)
    HMONITOR monitor = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
    if(!monitor)
    {
        throw std::runtime_error("Failed to get primary monitor. You may be usi"
            "ng a remote shell.");
    }
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(monitor, &mi);
    const int displayWidth = mi.rcMonitor.right - mi.rcMonitor.left;
    const int displayHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // Set window size in screen coordinates (not pixels).
    WindowWidth = 0.5*displayWidth;
    WindowHeight = 0.5*displayHeight;

    // Create window and initialize Direct3D.
    WNDCLASSEX PazWindow = {};
    PazWindow.cbSize = sizeof(PazWindow);
    PazWindow.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    PazWindow.lpfnWndProc = window_proc;
    PazWindow.hCursor = LoadCursor(nullptr, IDC_ARROW);
    PazWindow.lpszClassName = L"PazWindow";
    auto atom = RegisterClassEx(&PazWindow);
    WindowHandle = CreateWindowEx(window_ex_style(), MAKEINTATOM(atom),
        L"PAZ_Graphics Window", window_style(), CW_USEDEFAULT, CW_USEDEFAULT,
        WindowWidth, WindowHeight, nullptr, nullptr, nullptr, nullptr);
    if(!WindowHandle)
    {
        std::array<wchar_t, 256> buf;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buf.data(), buf.size(), nullptr);
        const std::string errMsg(buf.begin(), buf.end());
        throw std::runtime_error("Failed to create window: " + errMsg);
    }
    ShowWindow(WindowHandle, SW_SHOWNA);
    DXGI_SWAP_CHAIN_DESC swapChainDescriptor = {};
    swapChainDescriptor.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDescriptor.BufferDesc.RefreshRate.Denominator = 0;
    swapChainDescriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 1; //TEMP ?
    swapChainDescriptor.OutputWindow = WindowHandle;
    swapChainDescriptor.Windowed = true;
    static const D3D_FEATURE_LEVEL featureLvlReq = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL featureLvl;
    auto hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED, &featureLvlReq, 1,
        D3D11_SDK_VERSION, &swapChainDescriptor, &SwapChain, &Device,
        &featureLvl, &DeviceContext);
    if(hr)
    {
        throw std::runtime_error("Failed to initialize Direct3D (" +
            format_hresult(hr) + ").");
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

    // Use raw mouse input when cursor is disabled.
    const RAWINPUTDEVICE rid = {0x01, 0x02, 0, WindowHandle};
    if(!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        throw std::runtime_error("Failed to register raw input device.");
    }

    // Set up final blitting pass.
    ID3DBlob* vertBlob;
    ID3DBlob* fragBlob;
    ID3DBlob* error;
    hr = D3DCompile(QuadVertSrc.c_str(), QuadVertSrc.size(), nullptr, nullptr,
        nullptr, "main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vertBlob,
        &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile final vertex shader: " +
            std::string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    hr = Device->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->
        GetBufferSize(), nullptr, &quadVertShader);
    if(hr)
    {
        throw std::runtime_error("Failed to create final vertex shader (" +
            paz::format_hresult(hr) + ").");
    }
    hr = D3DCompile(QuadFragSrc.c_str(), QuadFragSrc.size(), nullptr, nullptr,
        nullptr, "main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &fragBlob,
        &error);
    if(hr)
    {
        throw std::runtime_error("Failed to compile final fragment shader: " +
            std::string(error ? static_cast<char*>(error->GetBufferPointer()) :
            "No error given."));
    }
    hr = Device->CreatePixelShader(fragBlob->GetBufferPointer(), fragBlob->
        GetBufferSize(), nullptr, &quadFragShader);
    if(hr)
    {
        throw std::runtime_error("Failed to create final fragment shader (" +
            paz::format_hresult(hr) + ").");
    }
    D3D11_BUFFER_DESC quadBufDescriptor = {};
    quadBufDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
    quadBufDescriptor.ByteWidth = sizeof(float)*QuadPos.size();
    quadBufDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = QuadPos.data();
    hr = Device->CreateBuffer(&quadBufDescriptor, &data, &quadBuf);
    if(hr)
    {
        throw std::runtime_error("Failed to create final vertex buffer (" +
            paz::format_hresult(hr) + ").");
    }
    D3D11_INPUT_ELEMENT_DESC inputElemDescriptor = {"ATTR", 0,
        DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
        D3D11_INPUT_PER_VERTEX_DATA, 0};
    hr = Device->CreateInputLayout(&inputElemDescriptor, 1, vertBlob->
        GetBufferPointer(), vertBlob->GetBufferSize(), &quadLayout);
    if(hr)
    {
        throw std::runtime_error("Failed to create final input layout (" + paz::
            format_hresult(hr) + ").");
    }
    D3D11_RASTERIZER_DESC rasterDescriptor = {};
    rasterDescriptor.FillMode = D3D11_FILL_SOLID;
    rasterDescriptor.CullMode = D3D11_CULL_NONE;
    rasterDescriptor.FrontCounterClockwise = true;
    rasterDescriptor.DepthClipEnable = true;
    hr = Device->CreateRasterizerState(&rasterDescriptor, &blitState);
    if(hr)
    {
        throw std::runtime_error("Failed to create final rasterizer state (" +
            paz::format_hresult(hr) + ").");
    }
    D3D11_BUFFER_DESC blitBufDescriptor = {};
    blitBufDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    blitBufDescriptor.ByteWidth = 16;
    blitBufDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    blitBufDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = Device->CreateBuffer(&blitBufDescriptor, nullptr, &blitBuf);
    if(hr)
    {
        throw std::runtime_error("Failed to create final constant buffer (" +
            paz::format_hresult(hr) + ").");
    }

    // Start recording frame time.
    frameStart = std::chrono::steady_clock::now();
}

void paz::Window::MakeFullscreen()
{
    initialize();

    if(!MonitorHandle)
    {
        acquire_monitor();

        PrevWidth = WindowWidth;
        PrevHeight = WindowHeight;

        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorHandle, &mi);

        DWORD style = GetWindowLong(WindowHandle, GWL_STYLE);
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= window_style();
        SetWindowLong(WindowHandle, GWL_STYLE, style);

        const UINT flags = SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_FRAMECHANGED;
        SetWindowPos(WindowHandle, HWND_NOTOPMOST, mi.rcMonitor.left, mi.
            rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.
            bottom - mi.rcMonitor.top, flags);
    }
}

void paz::Window::MakeWindowed()
{
    initialize();

    if(MonitorHandle)
    {
        release_monitor();

        DWORD style = GetWindowLong(WindowHandle, GWL_STYLE);
        style &= ~WS_POPUP;
        style |= window_style();
        SetWindowLong(WindowHandle, GWL_STYLE, style);

        RECT rect = {PrevX, PrevY, PrevX + PrevWidth, PrevY + PrevHeight};
        if(initialize().isWindows10Version1607OrGreater)
        {
            initialize().adjustWindowRectExForDpi(&rect, window_style(), FALSE,
                window_ex_style(), initialize().getDpiForWindow(WindowHandle));
        }
        else
        {
            AdjustWindowRectEx(&rect, window_style(), FALSE, window_ex_style());
        }

        const UINT flags = SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_FRAMECHANGED;
        SetWindowPos(WindowHandle, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);
    }
}

ID3D11Device* paz::d3d_device()
{
    return Device;
}

ID3D11DeviceContext* paz::d3d_context()
{
    return DeviceContext;
}

void paz::Window::SetTitle(const std::string& title)
{
    initialize();

    std::wstring wstr(title.begin(), title.end()); //TEMP - ASCII only
    SetWindowText(WindowHandle, wstr.c_str());
}

bool paz::Window::IsKeyWindow()
{
    initialize();

    return WindowIsKey;
}

bool paz::Window::IsFullscreen()
{
    initialize();

    return MonitorHandle;
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

bool paz::Window::MouseDown(MouseButton button)
{
    initialize();

    return ::MouseDown.at(static_cast<int>(button));
}

bool paz::Window::MousePressed(MouseButton button)
{
    initialize();

    return ::MousePressed.at(static_cast<int>(button));
}

bool paz::Window::MouseReleased(MouseButton button)
{
    initialize();

    return ::MouseReleased.at(static_cast<int>(button));
}

std::pair<double, double> paz::Window::MousePos()
{
    initialize();

    if(CursorDisabled)
    {
        return ::MousePos;
    }
    else
    {
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(WindowHandle, &pos);
        return {pos.x, WindowHeight - pos.y};
    }
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

    if(WindowIsKey)
    {
        if(!CursorDisabled && mode == CursorMode::Disable)
        {
            GetCursorPos(&PriorCursorPos);
            ScreenToClient(WindowHandle, &PriorCursorPos);
            center_cursor();
            enable_raw_mouse_motion();
            capture_cursor();
        }
        else if(CursorDisabled && mode != CursorMode::Disable)
        {
            disable_raw_mouse_motion();
            release_cursor();
            ClientToScreen(WindowHandle, &PriorCursorPos);
            SetCursorPos(PriorCursorPos.x, PriorCursorPos.y);
        }
    }

    if(mode == CursorMode::Normal)
    {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
    else if(mode == CursorMode::Hidden || mode == CursorMode::Disable)
    {
        SetCursor(nullptr);
    }
    else
    {
        throw std::runtime_error("Unknown cursor mode.");
    }

    CurCursorMode = mode;
    CursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    initialize();

    return FboAspectRatio;
}

void paz::resize_targets()
{
    // Release resources.
    RenderTargetView->Release();

    // Resize back buffer.
    auto hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if(hr)
    {
        throw std::runtime_error("Failed to resize back buffer (" +
            format_hresult(hr) + ").");
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

    for(auto n : initialize().renderTargets)
    {
        reinterpret_cast<Texture::Data*>(n)->resize(Window::ViewportWidth(),
            Window::ViewportHeight());
    }
}

bool paz::Window::Done()
{
    initialize();

    return ::Done;
}

void paz::Window::PollEvents()
{
    initialize();

    MSG msg;
    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_QUIT)
        {
            ::Done = true;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // HACK: Release modifier keys that the system did not emit KEYUP for
    // NOTE: Shift keys on Windows tend to "stick" when both are pressed as
    //       no key up message is generated by the first key release
    // NOTE: Windows key is not reported as released by the Win+V hotkey
    //       Other Win hotkeys are handled implicitly by _glfwInputWindowFocus
    //       because they change the input focus
    // NOTE: The other half of this is in the WM_*KEY* handler in windowProc
    HWND handle = GetActiveWindow();
    if(handle)
    {
        static constexpr std::array<std::pair<int, Key>, 4> keys =
        {{
            {VK_LSHIFT, Key::LeftShift},
            {VK_RSHIFT, Key::RightShift},
            {VK_LWIN, Key::LeftSuper},
            {VK_RWIN, Key::RightSuper}
        }};

        for(const auto& n : keys)
        {
            const int vk = n.first;
            const int idx = static_cast<int>(n.second);
            if((GetKeyState(vk)&0x8000))
            {
                continue;
            }
            if(!::KeyDown[idx])
            {
                continue;
            }
            ::KeyDown[idx] = false;
            ::KeyReleased[idx] = true;
        }
    }

    if(CursorDisabled)
    {
        VirtMousePos.first = WindowWidth/2;
        VirtMousePos.second = WindowHeight/2;
        center_cursor();
    }

    GamepadState state;
    if(poll_gamepad_state(state))
    {
        for(int i = 0; i < NumGamepadButtons; ++i)
        {
            if(state.buttons[i])
            {
                ::GamepadActive = true;
                ::MouseActive = false;
                if(!::GamepadDown[i])
                {
                    ::GamepadPressed[i] = true;
                }
                ::GamepadDown[i] = true;
            }
            else
            {
                if(::GamepadDown[i])
                {
                    ::GamepadActive = true;
                    ::MouseActive = false;
                    ::GamepadReleased[i] = true;
                }
                ::GamepadDown[i] = false;
            }
        }
        if(std::abs(state.axes[0]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftStick.first = state.axes[0];
        }
        if(std::abs(state.axes[1]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftStick.second = state.axes[1];
        }
        if(std::abs(state.axes[2]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightStick.first = state.axes[2];
        }
        if(std::abs(state.axes[3]) > 0.1)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightStick.second = state.axes[3];
        }
        if(state.axes[4] > -0.9)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadLeftTrigger = state.axes[4];
        }
        if(state.axes[5] > -0.9)
        {
            ::GamepadActive = true;
            ::MouseActive = false;
            ::GamepadRightTrigger = state.axes[5];
        }
    }
}

void paz::Window::EndFrame()
{
    initialize();

    FrameInProgress = false;

    DeviceContext->RSSetState(initialize().blitState);
    D3D11_VIEWPORT viewport = {};
    viewport.Width = ViewportWidth();
    viewport.Height = ViewportHeight();
    DeviceContext->RSSetViewports(1, &viewport);
    DeviceContext->OMSetRenderTargets(1, &RenderTargetView, nullptr);
    DeviceContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    DeviceContext->IASetInputLayout(initialize().quadLayout);
    const unsigned int stride = 2*sizeof(float);
    const unsigned int offset = 0;
    DeviceContext->IASetVertexBuffers(0, 1, &initialize().quadBuf, &stride,
        &offset);
    DeviceContext->VSSetShader(initialize().quadVertShader, nullptr, 0);
    DeviceContext->PSSetShader(initialize().quadFragShader, nullptr, 0);
    DeviceContext->PSSetShaderResources(0, 1, &final_framebuffer().
        colorAttachment(0)._data->_resourceView);
    DeviceContext->PSSetSamplers(0, 1, &final_framebuffer().colorAttachment(0).
        _data->_sampler);
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = DeviceContext->Map(initialize().blitBuf, 0,
        D3D11_MAP_WRITE_DISCARD, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map final fragment function constan"
            "t buffer (" + format_hresult(hr) + ").");
    }
    std::copy(&Gamma, &Gamma + 1, reinterpret_cast<float*>(mappedSr.pData));
    const float f = Dither ? 1.f : 0.f;
    std::copy(&f, &f + 1, reinterpret_cast<float*>(mappedSr.pData) + 1);
    DeviceContext->Unmap(initialize().blitBuf, 0);
    DeviceContext->PSSetConstantBuffers(0, 1, &initialize().blitBuf);
    DeviceContext->Draw(QuadPos.size()/2, 0);

    SwapChain->Present(1, 0);
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        initialize().frameStart).count()*1e-6;
    initialize().frameStart = now;
}

void paz::Window::Quit()
{
    initialize();

    ::Done = true;
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
    RECT area;
    GetWindowRect(WindowHandle, &area);
    MoveWindow(WindowHandle, area.left, area.top, area.right - area.left, area.
        bottom - area.top, TRUE);
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    MaxWidth = width;
    MaxHeight = height;
    RECT area;
    GetWindowRect(WindowHandle, &area);
    MoveWindow(WindowHandle, area.left, area.top, area.right - area.left, area.
        bottom - area.top, TRUE);
}

void paz::Window::MakeResizable()
{
    initialize();

    Resizable = true;
    update_styles();
}

void paz::Window::MakeNotResizable()
{
    initialize();

    Resizable = false;
    update_styles();
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
        if(MinWidth)
        {
            width = std::max(width, MinWidth);
        }
        if(MaxWidth)
        {
            width = std::min(width, MaxWidth);
        }
        if(MinHeight)
        {
            height = std::max(height, MinHeight);
        }
        if(MaxHeight)
        {
            height = std::min(height, MaxHeight);
        }
    }

    if(!MonitorHandle)
    {
        RECT rect = {0, 0, width, height};
        AdjustWindowRectEx(&rect, window_style(), FALSE, window_ex_style());
        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, rect.right - rect.left, rect.
            bottom - rect.top, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|
            SWP_NOZORDER);

        resize_targets();
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

    if(FrameInProgress)
    {
        throw std::logic_error("Cannot read pixels before ending frame.");
    }

    const auto width = ViewportWidth();
    const auto height = ViewportHeight();

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
        throw std::runtime_error("Failed to create staging texture (" +
            format_hresult(hr) + ").");
    }

    DeviceContext->CopySubresourceRegion(staging, 0, 0, 0, 0,
        final_framebuffer().colorAttachment(0)._data->_texture, 0, nullptr);

    D3D11_MAPPED_SUBRESOURCE mappedSr;
    hr = DeviceContext->Map(staging, 0, D3D11_MAP_READ, 0, &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map staging texture (" +
            format_hresult(hr) + ").");
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

    const HDC dc = GetDC(nullptr);
    const UINT xDpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(nullptr, dc);
    float xScale = static_cast<float>(xDpi)/USER_DEFAULT_SCREEN_DPI;
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
