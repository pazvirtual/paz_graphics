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
#include <thread>
#include <sstream>
#include <cstring>
#include <iostream>
#include <iomanip>

static constexpr GUID HidClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb,
    0x00, 0x11, 0x11, 0x00, 0x00, 0x30};

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
uint hash1(in uint x)
{
    x +=  (x << 10u);
    x = x^(x >>  6u);
    x +=  (x <<  3u);
    x = x^(x >> 11u);
    x +=  (x << 15u);
    return x;
}
uint hash2(in uint2 v)
{
    return hash1(v.x^hash1(v.y));
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
    return construct_float(hash2(asuint(v)));
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

static HWND _windowHandle;
static HMONITOR _monitorHandle;
static ID3D11Device* _device;
static ID3D11DeviceContext* _deviceContext;
static IDXGISwapChain* _swapChain;
static ID3D11RenderTargetView* _renderTargetView;
static int _windowWidth;
static int _windowHeight;
static bool _windowIsKey;
static bool _windowIsMinimized;
static bool _frameAction;
static bool _cursorTracked;
static int _prevX;
static int _prevY;
static int _prevHeight;
static int _prevWidth;
static int _fboWidth;
static int _fboHeight;
static float _fboAspectRatio;
static int _minWidth = 0;
static int _minHeight = 0;
static int _maxWidth = 0;
static int _maxHeight = 0;
static bool _resizable = true;
static bool _done;
static paz::CursorMode _cursorMode = paz::CursorMode::Normal;
static std::array<bool, paz::NumKeys> _keyDown;
static std::array<bool, paz::NumKeys> _keyPressed;
static std::array<bool, paz::NumKeys> _keyReleased;
static std::array<bool, paz::NumMouseButtons> _mouseDown;
static std::array<bool, paz::NumMouseButtons> _mousePressed;
static std::array<bool, paz::NumMouseButtons> _mouseReleased;
static std::pair<double, double> _mousePos;
static std::pair<double, double> _prevMousePos;
static std::pair<double, double> _virtMousePos;
static std::pair<double, double> _scrollOffset;
static std::array<bool, paz::NumGamepadButtons> _gamepadDown;
static std::array<bool, paz::NumGamepadButtons> _gamepadPressed;
static std::array<bool, paz::NumGamepadButtons> _gamepadReleased;
static std::pair<double, double> _gamepadLeftStick;
static std::pair<double, double> _gamepadRightStick;
static double _gamepadLeftTrigger = -1.;
static double _gamepadRightTrigger = -1.;
static std::vector<paz::Gamepad> _gamepads;
static bool _gamepadActive;
static bool _mouseActive;
static bool _cursorDisabled;
static POINT _priorCursorPos;
static bool _frameInProgress;
static bool _hidpiEnabled = true;
static bool _syncEnabled = true;
static float _gamma = 2.2;
static bool _dither;
static LPDIRECTINPUT8 _dinput8Interface;
static HDEVNOTIFY _deviceNotificationHandle;
static std::chrono::time_point<std::chrono::steady_clock> _frameStart;
static UINT(*_getDpiForWindow)(HWND);
static BOOL(*_adjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
static BOOL(*_setProcessDPIAware)();
static BOOL(*_setProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
static HRESULT(*_setProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
static LONG(*_rtlVerifyVersionInfo)(OSVERSIONINFOEX*, ULONG, ULONGLONG);
static HRESULT(*_directInput8Create)(HINSTANCE, DWORD, REFIID, LPVOID*,
    LPUNKNOWN);
static bool _isWindows10Version1703OrGreater;
static bool _isWindows10Version1607OrGreater;
static bool _isWindows8Point1OrGreater;
static bool _isWindowsVistaOrGreater;
static ID3D11VertexShader* _quadVertShader;
static ID3D11PixelShader* _quadFragShader;
static ID3D11Buffer* _quadBuf;
static ID3D11InputLayout* _quadLayout;
static ID3D11RasterizerState* _blitState;
static ID3D11Buffer* _blitBuf;

static const DIOBJECTDATAFORMAT ObjectDataFormats[] =
{
    {&GUID_XAxis, DIJOFS_X, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_YAxis, DIJOFS_Y, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_ZAxis, DIJOFS_Z, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_RxAxis, DIJOFS_RX, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_RyAxis, DIJOFS_RY, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_RzAxis, DIJOFS_RZ, DIDFT_AXIS|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        DIDOI_ASPECTPOSITION},
    {&GUID_Slider, DIJOFS_SLIDER(0), DIDFT_AXIS|DIDFT_OPTIONAL|
        DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION},
    {&GUID_Slider, DIJOFS_SLIDER(1), DIDFT_AXIS|DIDFT_OPTIONAL|
        DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION},
    {&GUID_POV, DIJOFS_POV(0), DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE, 0},
    {&GUID_POV, DIJOFS_POV(1), DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE, 0},
    {&GUID_POV, DIJOFS_POV(2), DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE, 0},
    {&GUID_POV, DIJOFS_POV(3), DIDFT_POV|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE, 0},
    {nullptr, DIJOFS_BUTTON(0), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(1), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(2), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(3), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(4), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(5), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(6), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(7), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(8), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(9), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(10), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(11), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(12), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(13), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(14), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(15), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(16), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(17), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(18), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(19), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(20), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(21), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(22), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(23), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(24), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(25), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(26), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(27), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(28), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(29), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(30), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0},
    {nullptr, DIJOFS_BUTTON(31), DIDFT_BUTTON|DIDFT_OPTIONAL|DIDFT_ANYINSTANCE,
        0}
};

static const DIDATAFORMAT DataFormat =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDFT_ABSAXIS,
    sizeof(DIJOYSTATE),
    sizeof(ObjectDataFormats)/sizeof(DIOBJECTDATAFORMAT),
    const_cast<LPDIOBJECTDATAFORMAT>(ObjectDataFormats)
};

namespace
{
    struct ObjectData
    {
        std::vector<paz::GamepadElement> axes;
        std::vector<paz::GamepadElement> buttons;
        std::vector<paz::GamepadElement> hats;
        IDirectInputDevice8* device;
    };
}

static DWORD window_style()
{
    DWORD style = WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
    if(_monitorHandle)
    {
        style |= WS_POPUP;
    }
    else
    {
        style |= WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION;
        if(_resizable)
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
    DWORD style = GetWindowLong(_windowHandle, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW|WS_POPUP);
    style |= window_style();

    RECT rect;
    GetClientRect(_windowHandle, &rect);

    AdjustWindowRectEx(&rect, style, FALSE, window_ex_style());

    ClientToScreen(_windowHandle, reinterpret_cast<POINT*>(&rect.left));
    ClientToScreen(_windowHandle, reinterpret_cast<POINT*>(&rect.right));
    SetWindowLong(_windowHandle, GWL_STYLE, style);
    SetWindowPos(_windowHandle, HWND_TOP, rect.left, rect.top, rect.right -
        rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED|SWP_NOACTIVATE|
        SWP_NOZORDER);
}

static BOOL CALLBACK object_callback(const DIDEVICEOBJECTINSTANCE* doi, void*
    user)
{
    auto& data = *reinterpret_cast<ObjectData*>(user);

    paz::GamepadElement elem;

    if(DIDFT_GETTYPE(doi->dwType)&DIDFT_AXIS)
    {
        if(!std::memcmp(&doi->guidType, &GUID_Slider, sizeof(GUID)))
        {
            throw std::logic_error("SLIDER NOT IMPLEMENTED");
        }
        else if(!std::memcmp(&doi->guidType, &GUID_XAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_X;
        }
        else if(!std::memcmp(&doi->guidType, &GUID_YAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_Y;
        }
        else if(!std::memcmp(&doi->guidType, &GUID_ZAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_Z;
        }
        else if(!std::memcmp(&doi->guidType, &GUID_RxAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_RX;
        }
        else if(!std::memcmp(&doi->guidType, &GUID_RyAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_RY;
        }
        else if(!std::memcmp(&doi->guidType, &GUID_RzAxis, sizeof(GUID)))
        {
            elem.idx = DIJOFS_RZ;
        }
        else
        {
            return DIENUM_CONTINUE;
        }

        DIPROPRANGE dipr = {};
        dipr.diph.dwSize = sizeof(dipr);
        dipr.diph.dwHeaderSize = sizeof(dipr.diph);
        dipr.diph.dwObj = doi->dwType;
        dipr.diph.dwHow = DIPH_BYID;
        dipr.lMin = -32768;
        dipr.lMax = 32767;

        const auto hr = IDirectInputDevice8_SetProperty(data.device,
            DIPROP_RANGE, &dipr.diph);
        if(hr)
        {
            return DIENUM_CONTINUE;
        }

        data.axes.push_back(elem);
    }
    else if(DIDFT_GETTYPE(doi->dwType)&DIDFT_BUTTON)
    {
        elem.idx = DIJOFS_BUTTON(data.buttons.size());
        data.buttons.push_back(elem);
    }
    else if(DIDFT_GETTYPE(doi->dwType)&DIDFT_POV)
    {
        elem.idx = DIJOFS_POV(data.hats.size());
        data.hats.push_back(elem);
    }

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK device_callback(const DIDEVICEINSTANCE* di, void*)
{
    // Check if this device was already connected.
    for(const auto& n : _gamepads)
    {
        if(!std::memcmp(&n.deviceGuid, &di->guidInstance, sizeof(GUID)))
        {
            return DIENUM_CONTINUE;
        }
    }

    // Get GUID to look up mapping.
    char name[256] = {'U', 'n', 'k', 'n', 'o', 'w', 'n', '\0'};
    char buf[33];
    if(!std::memcmp(di->guidProduct.Data4 + 2, "PIDVID", 6))
    {
        std::snprintf(buf, sizeof(buf), "03000000%02x%02x0000%02x%02x0000000000"
            "00", static_cast<std::uint8_t>(di->guidProduct.Data1), static_cast<
            std::uint8_t>(di->guidProduct.Data1 >> 8), static_cast<std::
            uint8_t>(di->guidProduct.Data1 >> 16), static_cast<std::uint8_t>(
            di->guidProduct.Data1 >> 24));
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "05000000%02x%02x%02x%02x%02x%02x%02x%0"
            "2x%02x%02x%02x00", name[0], name[1], name[2], name[3], name[4],
            name[5], name[6], name[7], name[8], name[9], name[10]);
    }
    std::string guid = buf;

    // Check if we have a mapping for this type of gamepad.
    if(!paz::gamepad_mappings().count(guid))
    {
        return DIENUM_CONTINUE;
    }

    IDirectInputDevice8* device;
    auto hr = IDirectInput8_CreateDevice(_dinput8Interface, di->guidInstance,
        &device, nullptr);
    if(hr)
    {
        throw std::runtime_error("Failed to create DirectInput8 device (" +
            paz::format_hresult(hr) + ").");
    }
    hr = IDirectInputDevice8_SetDataFormat(device, &DataFormat);
    if(hr)
    {
        throw std::runtime_error("Failed to set DirectInput8 data format (" +
            paz::format_hresult(hr) + ").");
    }

    DIDEVCAPS dc = {};
    dc.dwSize = sizeof(dc);
    hr = IDirectInputDevice8_GetCapabilities(device, &dc);
    if(hr)
    {
        throw std::runtime_error("Failed to get DirectInput8 device capabilitie"
            "s (" + paz::format_hresult(hr) + ").");
    }

    DIPROPDWORD dipd = {};
    dipd.diph.dwSize = sizeof(dipd);
    dipd.diph.dwHeaderSize = sizeof(dipd.diph);
    dipd.diph.dwHow = DIPH_DEVICE;
    dipd.dwData = DIPROPAXISMODE_ABS;
    hr = IDirectInputDevice8_SetProperty(device, DIPROP_AXISMODE, &dipd.diph);
    if(hr)
    {
        throw std::runtime_error("Failed to set DirectInput8 device axis mode ("
            + paz::format_hresult(hr) + ").");
        IDirectInputDevice8_Release(device);
    }

    // [axes, buttons, hats]
    ObjectData data = {};
    data.device = device;
    hr = IDirectInputDevice8_EnumObjects(device, object_callback, &data,
        DIDFT_AXIS|DIDFT_BUTTON|DIDFT_POV);
    if(hr)
    {
        throw std::runtime_error("Failed to enumerate DirectInput8 device objec"
            "ts (" + paz::format_hresult(hr) + ").");
        IDirectInputDevice8_Release(device);
    }

    std::sort(data.axes.begin(), data.axes.end());
    std::sort(data.buttons.begin(), data.buttons.end());
    std::sort(data.hats.begin(), data.hats.end());

    _gamepads.push_back({di->guidInstance, device, guid, name, data.axes, data.
        buttons, data.hats});

    return DIENUM_CONTINUE;
}

static void detect_gamepad_connection()
{
    const auto hr = IDirectInput8_EnumDevices(_dinput8Interface,
        DI8DEVCLASS_GAMECTRL, device_callback, nullptr, DIEDFL_ALLDEVICES);
    if(hr)
    {
        throw std::runtime_error("Failed to enumerate DirectInput8 devices (" +
            paz::format_hresult(hr) + ").");
    }
}

static void detect_gamepad_disconnection()
{
    for(const auto& n : _gamepads)
    {
        // Poll DirectInput8 device.
        DIJOYSTATE joyState = {};
        IDirectInputDevice8_Poll(n.device);
        auto hr = IDirectInputDevice8_GetDeviceState(n.device, sizeof(joyState),
            &joyState);
        if(hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
        {
            IDirectInputDevice8_Acquire(n.device);
            IDirectInputDevice8_Poll(n.device);
            hr = IDirectInputDevice8_GetDeviceState(n.device, sizeof(joyState),
                &joyState);
        }
        if(hr)
        {
            IDirectInputDevice8_Unacquire(n.device);
            IDirectInputDevice8_Release(n.device);
        }
    }
}

static bool get_gamepad_state(paz::GamepadState& state)
{
    if(_gamepads.empty())
    {
        return false;
    }

    // Poll DirectInput8 device.
    DIJOYSTATE joyState = {};
    IDirectInputDevice8_Poll(_gamepads[0].device);
    auto hr = IDirectInputDevice8_GetDeviceState(_gamepads[0].device, sizeof(
        joyState), &joyState);
    if(hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST)
    {
        IDirectInputDevice8_Acquire(_gamepads[0].device);
        IDirectInputDevice8_Poll(_gamepads[0].device);
        hr = IDirectInputDevice8_GetDeviceState(_gamepads[0].device, sizeof(
            joyState), &joyState);
    }
    if(hr)
    {
        IDirectInputDevice8_Unacquire(_gamepads[0].device);
        IDirectInputDevice8_Release(_gamepads[0].device);
        return false;
    }

    // Process axes.
    std::vector<double> axes(_gamepads[0].axes.size(), 0.);
    for(std::size_t i = 0; i < _gamepads[0].axes.size(); ++i)
    {
        const void* data = reinterpret_cast<const char*>(&joyState) + _gamepads[
            0].axes[i].idx;
        axes[i] = (*reinterpret_cast<const LONG*>(data) + 0.5)/32767.5;
    }

    // Process buttons.
    std::vector<bool> buttons(_gamepads[0].buttons.size());
    for(std::size_t i = 0; i < _gamepads[0].buttons.size(); ++i)
    {
        const void* data = reinterpret_cast<const char*>(&joyState) + _gamepads[
            0].buttons[i].idx;
        buttons[i] = *reinterpret_cast<const BYTE*>(data)&0x80;
    }

    // Process hats.
    std::vector<int> hats(_gamepads[0].hats.size());
    for(std::size_t i = 0; i < _gamepads[0].hats.size(); ++i)
    {
        const void* data = reinterpret_cast<const char*>(&joyState) + _gamepads[
            0].hats[i].idx;
        static constexpr std::array<int, 9> hatStates =
        {
            1,   // up
            1|2, // right-up
            2,   // right
            2|4, // right-down
            4,   // down
            4|8, // left-down
            8,   // left
            1|8, // left-up
            0    // centered
        };
        int stateIdx = LOWORD(*reinterpret_cast<const DWORD*>(data))/(45*
            DI_DEGREES);
        if(stateIdx < 0 || stateIdx > 8)
        {
            stateIdx = 8;
        }
        hats[i] = hatStates[stateIdx];
    }

    // Use mapping to get state.
    const auto& mapping = paz::gamepad_mappings().at(_gamepads[0].guid);
    for(int i = 0; i < paz::NumGamepadButtons; ++i)
    {
        const auto e = mapping.buttons[i];
        switch(e.type)
        {
            case paz::GamepadElementType::Axis:
            {
                const float val = axes[e.idx]*e.axisScale + e.axisOffset;
                if(e.axisOffset < 0. || (!e.axisOffset && e.axisScale > 0.))
                {
                    state.buttons[i] = val >= 0.;
                }
                else
                {
                    state.buttons[i] = val <= 0.;
                }
                break;
            }
            case paz::GamepadElementType::HatBit:
            {
                const unsigned int hat = e.idx >> 4;
                const unsigned int bit = e.idx&0xf;
                state.buttons[i] = hats[hat]&bit;
                break;
            }
            case paz::GamepadElementType::Button:
            {
                state.buttons[i] = buttons[e.idx];
                break;
            }
        }
    }
    for(int i = 0; i < 6; ++i)
    {
        const auto e = mapping.axes[i];
        switch(e.type)
        {
            case paz::GamepadElementType::Axis:
            {
                const double val = axes[e.idx]*e.axisScale + e.axisOffset;
                state.axes[i] = std::max(-1., std::min(1., val));
                break;
            }
            case paz::GamepadElementType::HatBit:
            {
                const unsigned int hat = e.idx >> 4;
                const unsigned int bit = e.idx&0xf;
                state.axes[i] = hats[hat]&bit ? 1. : 1.;
                break;
            }
            case paz::GamepadElementType::Button:
            {
                state.axes[i] = 2.*buttons[e.idx] - 1.;
                break;
            }
        }
    }

    return true;
}

static double PrevFrameTime = 1./60.;

static void center_cursor()
{
    POINT pos = {_windowWidth/2, _windowHeight/2};
    ClientToScreen(_windowHandle, &pos);
    SetCursorPos(pos.x, pos.y);
}

static void capture_cursor()
{
    RECT clipRect;
    GetClientRect(_windowHandle, &clipRect);
    ClientToScreen(_windowHandle, reinterpret_cast<POINT*>(&clipRect.left));
    ClientToScreen(_windowHandle, reinterpret_cast<POINT*>(&clipRect.right));
    ClipCursor(&clipRect);
}

static void release_cursor()
{
    ClipCursor(nullptr);
}

static void enable_raw_mouse_motion()
{
    const RAWINPUTDEVICE rid = {0x01, 0x02, 0, _windowHandle};
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
    GetCursorPos(&_priorCursorPos);
    ScreenToClient(_windowHandle, &_priorCursorPos);
    center_cursor();
    SetCursor(nullptr);
    capture_cursor();
    enable_raw_mouse_motion();
}

static void enable_cursor()
{
    disable_raw_mouse_motion();
    release_cursor();
    _prevMousePos.first = _priorCursorPos.x;
    _prevMousePos.second = _windowHeight - _priorCursorPos.y;
    ClientToScreen(_windowHandle, &_priorCursorPos);
    SetCursorPos(_priorCursorPos.x, _priorCursorPos.y);
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

static void acquire_monitor()
{
    if(!_monitorHandle)
    {
        SetThreadExecutionState(ES_CONTINUOUS|ES_DISPLAY_REQUIRED);
        _monitorHandle = MonitorFromPoint({}, MONITOR_DEFAULTTOPRIMARY);
    }
}

static void release_monitor()
{
    if(_monitorHandle)
    {
        SetThreadExecutionState(ES_CONTINUOUS);
        _monitorHandle = nullptr;
    }
}

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM
    lParam)
{
    if(!_windowHandle)
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
                    _frameAction = true;
                }
            }
            break;
        }
        case WM_CAPTURECHANGED:
        {
            // Disable the cursor once the caption button action has been
            // completed or canceled.
            if(!lParam && _frameAction)
            {
                if(_cursorDisabled)
                {
                    disable_cursor();
                }
                _frameAction = false;
            }
            break;
        }
        case WM_SETFOCUS:
        {
            _windowIsKey = true;
            // Do not disable cursor while the user is interacting with a
            // caption button.
            if(_frameAction)
            {
                break;
            }
            if(_cursorDisabled)
            {
                disable_cursor();
            }
            return 0;
        }
        case WM_KILLFOCUS:
        {
            _windowIsKey = false;
            if(_cursorDisabled)
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
                    if(_monitorHandle)
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
            _done = true;
            return 0;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const bool released = HIWORD(lParam)&KF_UP;
            int key = HIWORD(lParam)&(KF_EXTENDED|0xff);
            if(!key)
            {
                key = MapVirtualKey(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC);
            }
            if(key == 0x54)
            {
                key = 0x137;
            }
            else if(key == 0x146)
            {
                key = 0x45;
            }
            else if(key == 0x136)
            {
                key = 0x36;
            }
            paz::Key k = paz::convert_keycode(key);
            if(wParam == VK_CONTROL)
            {
                if(HIWORD(lParam)&KF_EXTENDED)
                {
                    k = paz::Key::RightControl;
                }
                else
                {
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
                                break;
                            }
                        }
                    }
                    k = paz::Key::LeftControl;
                }
            }
            else if(wParam == VK_PROCESSKEY)
            {
                break;
            }
            if(released && wParam == VK_SHIFT)
            {
                _gamepadActive = false;
                _mouseActive = false;
                static constexpr int l = static_cast<int>(paz::Key::LeftShift);
                static constexpr int r = static_cast<int>(paz::Key::RightShift);
                _keyDown[l] = false;
                _keyReleased[l] = true;
                _keyDown[r] = false;
                _keyReleased[r] = true;
            }
            else if(wParam == VK_SNAPSHOT)
            {
                _gamepadActive = false;
                _mouseActive = false;
                const int i = static_cast<int>(k);
                _keyDown[i] = false;
                _keyPressed[i] = true;
                _keyReleased[i] = true;
            }
            else if(k == paz::Key::Unknown)
            {
                break;
            }
            else
            {
                _gamepadActive = false;
                _mouseActive = false;
                const int i = static_cast<int>(k);
                if(released)
                {
                    _keyDown[i] = false;
                    _keyReleased[i] = true;
                }
                else
                {
                    _keyDown[i] = true;
                    _keyPressed[i] = true;
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
            _gamepadActive = false;
            _mouseActive = true;
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
            if(std::none_of(_mouseDown.begin(), _mouseDown.end(), [](bool x){
                return x; }))
            {
                SetCapture(hWnd);
            }
            if(uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg ==
                WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN)
            {
                _mouseDown[button] = true;
                _mousePressed[button] = true;
            }
            else
            {
                _mouseDown[button] = false;
                _mouseReleased[button] = true;
            }
            if(std::none_of(_mouseDown.begin(), _mouseDown.end(), [](bool x){
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
            if(!_cursorTracked)
            {
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = _windowHandle;
                TrackMouseEvent(&tme);
                _cursorTracked = true;
            }
            if(_cursorDisabled)
            {
                break;
            }
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);
            if(_virtMousePos.first != x || _virtMousePos.second != y)
            {
                _gamepadActive = false;
                _mouseActive = true;
                _virtMousePos.first = x;
                _virtMousePos.second = y;
                _mousePos.first = _virtMousePos.first;
                _mousePos.second = _windowHeight - _virtMousePos.second;
            }
            _prevMousePos.first = x;
            _prevMousePos.second = y;
            return 0;
        }
        case WM_INPUT:
        {
            if(!_cursorDisabled)
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
                deltaX = data.data.mouse.lLastX - _prevMousePos.first;
                deltaY = data.data.mouse.lLastY - _prevMousePos.second;
            }
            else
            {
                deltaX = data.data.mouse.lLastX;
                deltaY = data.data.mouse.lLastY;
            }
            const double x = _virtMousePos.first + deltaX;
            const double y = _virtMousePos.second + deltaY;
            if(_virtMousePos.first != x || _virtMousePos.second != y)
            {
                _gamepadActive = false;
                _mouseActive = true;
                _virtMousePos.first = x;
                _virtMousePos.second = y;
                _mousePos.first = _virtMousePos.first - _windowWidth/2;
                _mousePos.second = _windowHeight/2 - _virtMousePos.second;
            }
            _prevMousePos.first += deltaX;
            _prevMousePos.second += deltaY;
            break;
        }
        case WM_MOUSELEAVE:
        {
            _gamepadActive = false;
            _mouseActive = true;
            _cursorTracked = false;
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            _gamepadActive = false;
            _mouseActive = true;
            _scrollOffset.second = static_cast<double>(GET_WHEEL_DELTA_WPARAM(
                wParam))/WHEEL_DELTA;
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            _gamepadActive = false;
            _mouseActive = true;
            _scrollOffset.first = -static_cast<double>(GET_WHEEL_DELTA_WPARAM(
                wParam))/WHEEL_DELTA;
            return 0;
        }
        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP:
        {
            if(_frameAction)
            {
                break;
            }
            // Enable the cursor while the user is moving or resizing the window
            // or using the window menu.
            if(_cursorDisabled)
            {
                enable_cursor();
            }
            break;
        }
        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP:
        {
            if(_frameAction)
            {
                break;
            }
            // Disable the cursor once the user is done moving or resizing the
            // window or using the menu.
            if(_cursorDisabled)
            {
                disable_cursor();
            }
            break;
        }
        case WM_SIZE:
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            const bool minimized = wParam == SIZE_MINIMIZED;
            if(_cursorDisabled)
            {
                capture_cursor();
            }
            if(newWidth != _windowWidth || newHeight != _windowHeight)
            {
                _windowWidth = newWidth;
                _windowHeight = newHeight;
                if(!minimized)
                {
                    _fboWidth = newWidth; //TEMP - verify DPI scaling
                    _fboHeight = newHeight; //TEMP - verify DPI scaling
                    _fboAspectRatio = static_cast<float>(_fboWidth)/_fboHeight;
                    if(_device)
                    {
                        paz::resize_targets();
                    }
                }
            }
            if(_monitorHandle && _windowIsMinimized != minimized)
            {
                if(minimized)
                {
                    release_monitor();
                }
                else
                {
                    acquire_monitor();
                    MONITORINFO mi;
                    mi.cbSize = sizeof(mi);
                    GetMonitorInfo(_monitorHandle, &mi);
                    SetWindowPos(_windowHandle, HWND_TOPMOST, mi.rcMonitor.left,
                        mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.
                        left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);
                }
            }
            _windowIsMinimized = minimized;
            return 0;
        }
        case WM_MOVE:
        {
            if(!_monitorHandle)
            {
                _prevX = static_cast<int>(GET_X_LPARAM(lParam));
                _prevY = static_cast<int>(GET_Y_LPARAM(lParam));
                if(_cursorDisabled)
                {
                    capture_cursor();
                }
            }
            return 0;
        }
        case WM_GETMINMAXINFO:
        {
            RECT frame = {};
            MINMAXINFO& mmi = *reinterpret_cast<MINMAXINFO*>(lParam);
            if(_monitorHandle)
            {
                break;
            }
            if(_isWindows10Version1607OrGreater)
            {
                _adjustWindowRectExForDpi(&frame, window_style(), FALSE,
                    window_ex_style(), _getDpiForWindow(_windowHandle));
            }
            else
            {
                AdjustWindowRectEx(&frame, window_style(), FALSE,
                    window_ex_style());
            }
            if(_minWidth && _minHeight)
            {
                mmi.ptMinTrackSize.x = _minWidth + frame.right - frame.left;
                mmi.ptMinTrackSize.y = _minHeight + frame.bottom - frame.top;
            }
            if(_maxWidth && _maxHeight)
            {
                mmi.ptMaxTrackSize.x = _maxWidth + frame.right - frame.left;
                mmi.ptMaxTrackSize.y = _maxHeight + frame.bottom - frame.top;
            }
            return 0;
        }
        case WM_ERASEBKGND:
        {
            return TRUE;
        }
        case WM_DWMCOMPOSITIONCHANGED:
        case WM_DWMCOLORIZATIONCOLORCHANGED:
        {
            return 0;
        }
        case WM_GETDPISCALEDSIZE:
        {
            // Adjust the window size to keep the content area size constant.
            if(_isWindows10Version1703OrGreater)
            {
                RECT source = {}, target = {};
                SIZE& size = *reinterpret_cast<SIZE*>(lParam);
                _adjustWindowRectExForDpi(&source, window_style(), FALSE,
                    window_ex_style(), _getDpiForWindow(_windowHandle));
                _adjustWindowRectExForDpi(&target, window_style(), FALSE,
                    window_ex_style(), LOWORD(wParam));
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
            if(!_monitorHandle && _isWindows10Version1703OrGreater)
            {
                RECT* suggested = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(_windowHandle, HWND_TOP, suggested->left,
                    suggested->top, suggested->right - suggested->left,
                    suggested->bottom - suggested->top, SWP_NOACTIVATE|
                    SWP_NOZORDER);
            }
            if(_device)
            {
                paz::resize_targets();
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if(LOWORD(lParam) == HTCLIENT)
            {
                if(_cursorMode == paz::CursorMode::Normal)
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
        case WM_DEVICECHANGE:
        {
            if(wParam == DBT_DEVICEARRIVAL)
            {
                DEV_BROADCAST_HDR* dbh = reinterpret_cast<DEV_BROADCAST_HDR*>(
                    lParam);
                if(dbh && dbh->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    detect_gamepad_connection();
                }
            }
            else if(wParam == DBT_DEVICEREMOVECOMPLETE)
            {
                DEV_BROADCAST_HDR* dbh = reinterpret_cast<DEV_BROADCAST_HDR*>(
                    lParam);
                if(dbh && dbh->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    detect_gamepad_disconnection();
                }
            }
            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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

paz::Initializer::~Initializer() {}

paz::Initializer::Initializer()
{
    // Load functions from DLLs.
    const HMODULE user32 = LoadLibrary(L"user32.dll");
    if(!user32)
    {
        throw std::runtime_error("Failed to load \"user32.dll\".");
    }
    _getDpiForWindow = reinterpret_cast<decltype(_getDpiForWindow)>(
        GetProcAddress(user32, "GetDpiForWindow"));
    _adjustWindowRectExForDpi = reinterpret_cast<decltype(
        _adjustWindowRectExForDpi)>(GetProcAddress(user32,
        "AdjustWindowRectExForDpi"));
    _setProcessDPIAware = reinterpret_cast<decltype(_setProcessDPIAware)>(
        GetProcAddress(user32, "SetProcessDPIAware"));
    _setProcessDpiAwarenessContext = reinterpret_cast<decltype(
        _setProcessDpiAwarenessContext)>(GetProcAddress(user32,
        "SetProcessDpiAwarenessContext"));
    const HMODULE shcore = LoadLibrary(L"shcore.dll");
    if(!shcore)
    {
        throw std::runtime_error("Failed to load \"shcore.dll\".");
    }
    _setProcessDpiAwareness = reinterpret_cast<decltype(
        _setProcessDpiAwareness)>(GetProcAddress(shcore,
        "SetProcessDpiAwareness"));
    const HMODULE ntdll = LoadLibrary(L"ntdll.dll");
    if(!ntdll)
    {
        throw std::runtime_error("Failed to load \"ntdll.dll\".");
    }
    _rtlVerifyVersionInfo = reinterpret_cast<decltype(_rtlVerifyVersionInfo)>(
        GetProcAddress(ntdll, "RtlVerifyVersionInfo"));
    const HMODULE dinput8 = LoadLibrary(L"dinput8.dll");
    if(!dinput8)
    {
        throw std::runtime_error("Failed to load \"dinput8.dll\".");
    }
    _directInput8Create = reinterpret_cast<decltype(_directInput8Create)>(
        GetProcAddress(dinput8, "DirectInput8Create"));

    // Check Windows version.
    OSVERSIONINFOEX osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_VISTA);
    osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_VISTA);
    ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION,
        VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    _isWindowsVistaOrGreater = !_rtlVerifyVersionInfo(&osvi, VER_MAJORVERSION|
        VER_MINORVERSION, cond);
    osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_WINBLUE);
    osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_WINBLUE);
    _isWindows8Point1OrGreater = !_rtlVerifyVersionInfo(&osvi, VER_MAJORVERSION|
        VER_MINORVERSION, cond);
    osvi.dwMajorVersion = 10;
    osvi.dwMinorVersion = 0;
    osvi.dwBuildNumber = 14393;
    cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    _isWindows10Version1607OrGreater = !_rtlVerifyVersionInfo(&osvi,
        VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER, cond);
    osvi.dwBuildNumber = 15063;
    _isWindows10Version1703OrGreater = !_rtlVerifyVersionInfo(&osvi,
        VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER, cond);

    // Set up DPI awareness.
    if(_isWindows10Version1703OrGreater)
    {
        _setProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
    else if(_isWindows8Point1OrGreater)
    {
        _setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    }
    else if(_isWindowsVistaOrGreater)
    {
        _setProcessDPIAware();
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
    _windowWidth = 0.5*displayWidth;
    _windowHeight = 0.5*displayHeight;

    // Create window.
    WNDCLASSEX PazWindow = {};
    PazWindow.cbSize = sizeof(PazWindow);
    PazWindow.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    PazWindow.lpfnWndProc = window_proc;
    PazWindow.hCursor = LoadCursor(nullptr, IDC_ARROW);
    PazWindow.lpszClassName = L"PazWindow";
    auto atom = RegisterClassEx(&PazWindow);
    _windowHandle = CreateWindowEx(window_ex_style(), MAKEINTATOM(atom),
        L"PAZ_Graphics Window", window_style(), CW_USEDEFAULT, CW_USEDEFAULT,
        _windowWidth, _windowHeight, nullptr, nullptr, nullptr, nullptr);
    if(!_windowHandle)
    {
        throw std::runtime_error("Failed to create window: " +
            get_last_error());
    }

    // Show and focus window.
    ShowWindow(_windowHandle, SW_SHOWNA);
    BringWindowToTop(_windowHandle);
    SetForegroundWindow(_windowHandle);
    SetFocus(_windowHandle);

    // Initialize Direct3D.
    DXGI_SWAP_CHAIN_DESC swapChainDescriptor = {};
    swapChainDescriptor.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDescriptor.BufferDesc.RefreshRate.Denominator = 0;
    swapChainDescriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 1; //TEMP ?
    swapChainDescriptor.OutputWindow = _windowHandle;
    swapChainDescriptor.Windowed = true;
    static const D3D_FEATURE_LEVEL featureLvlReq = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL featureLvl;
    auto hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED, &featureLvlReq, 1,
        D3D11_SDK_VERSION, &swapChainDescriptor, &_swapChain, &_device,
        &featureLvl, &_deviceContext);
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
    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<
        void**>(&framebuffer));
    if(hr)
    {
        throw std::runtime_error("Failed to get default framebuffer.");
    }
    hr = _device->CreateRenderTargetView(framebuffer, nullptr,
        &_renderTargetView);
    if(hr)
    {
        throw std::runtime_error("Failed to create default framebuffer view.");
    }
    framebuffer->Release();

    // Use raw mouse input when cursor is disabled.
    const RAWINPUTDEVICE rid = {0x01, 0x02, 0, _windowHandle};
    if(!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        throw std::runtime_error("Failed to register raw input device.");
    }

    // Register for HID device notifications.
    DEV_BROADCAST_DEVICEINTERFACE dbi = {};
    dbi.dbcc_size = sizeof(dbi);
    dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbi.dbcc_classguid = HidClassGuid;
    _deviceNotificationHandle = RegisterDeviceNotification(_windowHandle,
        reinterpret_cast<DEV_BROADCAST_HDR*>(&dbi),
        DEVICE_NOTIFY_WINDOW_HANDLE);

    // Create DirectInput8 interface to handle controllers.
    hr = _directInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
        IID_IDirectInput8, reinterpret_cast<void**>(&_dinput8Interface),
        nullptr);
    if(hr)
    {
        throw std::runtime_error("Failed to create DirectInput8 interface (" +
            format_hresult(hr) + ").");
    }

    // Get any initially-attached gamepads.
    detect_gamepad_connection();

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
    hr = _device->CreateVertexShader(vertBlob->GetBufferPointer(), vertBlob->
        GetBufferSize(), nullptr, &_quadVertShader);
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
    hr = _device->CreatePixelShader(fragBlob->GetBufferPointer(), fragBlob->
        GetBufferSize(), nullptr, &_quadFragShader);
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
    hr = _device->CreateBuffer(&quadBufDescriptor, &data, &_quadBuf);
    if(hr)
    {
        throw std::runtime_error("Failed to create final vertex buffer (" +
            paz::format_hresult(hr) + ").");
    }
    D3D11_INPUT_ELEMENT_DESC inputElemDescriptor = {"ATTR", 0,
        DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,
        D3D11_INPUT_PER_VERTEX_DATA, 0};
    hr = _device->CreateInputLayout(&inputElemDescriptor, 1, vertBlob->
        GetBufferPointer(), vertBlob->GetBufferSize(), &_quadLayout);
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
    hr = _device->CreateRasterizerState(&rasterDescriptor, &_blitState);
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
    hr = _device->CreateBuffer(&blitBufDescriptor, nullptr, &_blitBuf);
    if(hr)
    {
        throw std::runtime_error("Failed to create final constant buffer (" +
            paz::format_hresult(hr) + ").");
    }

    // Start recording frame time.
    _frameStart = std::chrono::steady_clock::now();
}

void paz::Window::MakeFullscreen()
{
    initialize();

    if(!_monitorHandle)
    {
        acquire_monitor();

        _prevWidth = _windowWidth;
        _prevHeight = _windowHeight;

        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(_monitorHandle, &mi);

        DWORD style = GetWindowLong(_windowHandle, GWL_STYLE);
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= window_style();
        SetWindowLong(_windowHandle, GWL_STYLE, style);

        SetWindowPos(_windowHandle, HWND_NOTOPMOST, mi.rcMonitor.left, mi.
            rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.
            bottom - mi.rcMonitor.top, SWP_SHOWWINDOW|SWP_NOACTIVATE|
            SWP_NOCOPYBITS|SWP_FRAMECHANGED);
    }
}

void paz::Window::MakeWindowed()
{
    initialize();

    if(_monitorHandle)
    {
        release_monitor();

        DWORD style = GetWindowLong(_windowHandle, GWL_STYLE);
        style &= ~WS_POPUP;
        style |= window_style();
        SetWindowLong(_windowHandle, GWL_STYLE, style);

        RECT rect = {_prevX, _prevY, _prevX + _prevWidth, _prevY + _prevHeight};
        if(_isWindows10Version1607OrGreater)
        {
            _adjustWindowRectExForDpi(&rect, window_style(), FALSE,
                window_ex_style(), _getDpiForWindow(_windowHandle));
        }
        else
        {
            AdjustWindowRectEx(&rect, window_style(), FALSE, window_ex_style());
        }

        SetWindowPos(_windowHandle, HWND_NOTOPMOST, rect.left, rect.top, rect.
            right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE|
            SWP_NOCOPYBITS|SWP_FRAMECHANGED);
    }
}

ID3D11Device* paz::d3d_device()
{
    return _device;
}

ID3D11DeviceContext* paz::d3d_context()
{
    return _deviceContext;
}

void paz::Window::SetTitle(const std::string& title)
{
    initialize();

    SetWindowText(_windowHandle, utf8_to_wstring(title).c_str());
}

bool paz::Window::IsKeyWindow()
{
    initialize();

    return _windowIsKey;
}

bool paz::Window::IsFullscreen()
{
    initialize();

    return _monitorHandle;
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
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(_windowHandle, &pos);
        return {pos.x, _windowHeight - pos.y};
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

    if(_windowIsKey)
    {
        if(!_cursorDisabled && mode == CursorMode::Disable)
        {
            GetCursorPos(&_priorCursorPos);
            ScreenToClient(_windowHandle, &_priorCursorPos);
            center_cursor();
            enable_raw_mouse_motion();
            capture_cursor();
        }
        else if(_cursorDisabled && mode != CursorMode::Disable)
        {
            disable_raw_mouse_motion();
            release_cursor();
            ClientToScreen(_windowHandle, &_priorCursorPos);
            SetCursorPos(_priorCursorPos.x, _priorCursorPos.y);
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

    _cursorMode = mode;
    _cursorDisabled = (mode == CursorMode::Disable);
}

float paz::Window::AspectRatio()
{
    initialize();

    return _fboAspectRatio;
}

void paz::resize_targets()
{
    // Release resources.
    _renderTargetView->Release();

    // Resize back buffer.
    auto hr = _swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if(hr)
    {
        throw std::runtime_error("Failed to resize back buffer (" +
            format_hresult(hr) + ").");
    }

    // Regenerate render target view.
    ID3D11Texture2D* framebuffer;
    hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<
        void**>(&framebuffer));
    if(hr)
    {
        throw std::runtime_error("Failed to get default framebuffer.");
    }
    hr = _device->CreateRenderTargetView(framebuffer, nullptr,
        &_renderTargetView);
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

    return _done;
}

void paz::Window::PollEvents()
{
    initialize();

    MSG msg;
    while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if(msg.message == WM_QUIT)
        {
            _done = true;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

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
            if(!_keyDown[idx])
            {
                continue;
            }
            _keyDown[idx] = false;
            _keyReleased[idx] = true;
        }
    }

    if(_cursorDisabled)
    {
        _virtMousePos.first = _windowWidth/2;
        _virtMousePos.second = _windowHeight/2;
        center_cursor();
    }

    GamepadState state;
    if(get_gamepad_state(state))
    {
        for(int i = 0; i < NumGamepadButtons; ++i)
        {
            if(state.buttons[i])
            {
                _gamepadActive = true;
                _mouseActive = false;
                if(!_gamepadDown[i])
                {
                    _gamepadPressed[i] = true;
                }
                _gamepadDown[i] = true;
            }
            else
            {
                if(_gamepadDown[i])
                {
                    _gamepadActive = true;
                    _mouseActive = false;
                    _gamepadReleased[i] = true;
                }
                _gamepadDown[i] = false;
            }
        }
        if(std::abs(state.axes[0]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.first = state.axes[0];
        }
        if(std::abs(state.axes[1]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftStick.second = state.axes[1];
        }
        if(std::abs(state.axes[2]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.first = state.axes[2];
        }
        if(std::abs(state.axes[3]) > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightStick.second = state.axes[3];
        }
#if 0 //TEMP - need to use XInput to handle XBox controller triggers independently
        if(state.axes[4] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftTrigger = state.axes[4];
        }
        if(state.axes[5] > -0.9)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightTrigger = state.axes[5];
        }
#else
        if(state.axes[4] > 0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadLeftTrigger = 2.*state.axes[4] - 1.;
        }
        else if(state.axes[4] < -0.1)
        {
            _gamepadActive = true;
            _mouseActive = false;
            _gamepadRightTrigger = -2.*state.axes[4] - 1.;
        }
#endif
    }
}

void paz::Window::EndFrame()
{
    initialize();

    _frameInProgress = false;

    _deviceContext->RSSetState(_blitState);
    D3D11_VIEWPORT viewport = {};
    viewport.Width = ViewportWidth();
    viewport.Height = ViewportHeight();
    _deviceContext->RSSetViewports(1, &viewport);
    _deviceContext->OMSetRenderTargets(1, &_renderTargetView, nullptr);
    _deviceContext->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _deviceContext->IASetInputLayout(_quadLayout);
    const unsigned int stride = 2*sizeof(float);
    const unsigned int offset = 0;
    _deviceContext->IASetVertexBuffers(0, 1, &_quadBuf, &stride, &offset);
    _deviceContext->VSSetShader(_quadVertShader, nullptr, 0);
    _deviceContext->PSSetShader(_quadFragShader, nullptr, 0);
    _deviceContext->PSSetShaderResources(0, 1, &final_framebuffer().
        colorAttachment(0)._data->_resourceView);
    _deviceContext->PSSetSamplers(0, 1, &final_framebuffer().colorAttachment(0).
        _data->_sampler);
    D3D11_MAPPED_SUBRESOURCE mappedSr;
    const auto hr = _deviceContext->Map(_blitBuf, 0, D3D11_MAP_WRITE_DISCARD, 0,
        &mappedSr);
    if(hr)
    {
        throw std::runtime_error("Failed to map final fragment function constan"
            "t buffer (" + format_hresult(hr) + ").");
    }
    std::copy(&_gamma, &_gamma + 1, reinterpret_cast<float*>(mappedSr.pData));
    const float f = _dither ? 1.f : 0.f;
    std::copy(&f, &f + 1, reinterpret_cast<float*>(mappedSr.pData) + 1);
    _deviceContext->Unmap(_blitBuf, 0);
    _deviceContext->PSSetConstantBuffers(0, 1, &_blitBuf);
    _deviceContext->Draw(QuadPos.size()/2, 0);

    _swapChain->Present(_syncEnabled, 0);
    reset_events();
    const auto now = std::chrono::steady_clock::now();
    PrevFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(now -
        _frameStart).count()*1e-6;
    _frameStart = now;

    // Keep framerate down while minimized (not capped otherwise).
    if(_windowIsMinimized)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void paz::Window::Quit()
{
    initialize();

    _done = true;
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
    RECT area;
    GetWindowRect(_windowHandle, &area);
    MoveWindow(_windowHandle, area.left, area.top, area.right - area.left, area.
        bottom - area.top, TRUE);
}

void paz::Window::SetMaxSize(int width, int height)
{
    initialize();

    _maxWidth = width;
    _maxHeight = height;
    RECT area;
    GetWindowRect(_windowHandle, &area);
    MoveWindow(_windowHandle, area.left, area.top, area.right - area.left, area.
        bottom - area.top, TRUE);
}

void paz::Window::MakeResizable()
{
    initialize();

    _resizable = true;
    update_styles();
}

void paz::Window::MakeNotResizable()
{
    initialize();

    _resizable = false;
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
        if(_minWidth)
        {
            width = std::max(width, _minWidth);
        }
        if(_maxWidth)
        {
            width = std::min(width, _maxWidth);
        }
        if(_minHeight)
        {
            height = std::max(height, _minHeight);
        }
        if(_maxHeight)
        {
            height = std::min(height, _maxHeight);
        }
    }

    if(!_monitorHandle)
    {
        RECT rect = {0, 0, width, height};
        AdjustWindowRectEx(&rect, window_style(), FALSE, window_ex_style());
        SetWindowPos(_windowHandle, HWND_TOP, 0, 0, rect.right - rect.left,
            rect.bottom - rect.top, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|
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

    if(_frameInProgress)
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

    _deviceContext->CopySubresourceRegion(staging, 0, 0, 0, 0,
        final_framebuffer().colorAttachment(0)._data->_texture, 0, nullptr);

    D3D11_MAPPED_SUBRESOURCE mappedSr;
    hr = _deviceContext->Map(staging, 0, D3D11_MAP_READ, 0, &mappedSr);
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

    _deviceContext->Unmap(staging, 0);

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

    const HDC dc = GetDC(nullptr);
    const UINT xDpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(nullptr, dc);
    float xScale = static_cast<float>(xDpi)/USER_DEFAULT_SCREEN_DPI;
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

    return D3D11_DEFAULT_MAX_ANISOTROPY; //TEMP - should check if hardware supports higher
}

void paz::Window::DisableSync()
{
    initialize();

    _syncEnabled = false;
}

void paz::Window::EnableSync()
{
    initialize();

    _syncEnabled = true;
}

bool paz::Window::SyncEnabled()
{
    initialize();

    return _syncEnabled;
}

bool paz::Window::SyncToggleSupported()
{
    initialize();

    return true;
}

#endif
