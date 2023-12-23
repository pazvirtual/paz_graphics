#ifndef PAZ_GRAPHICS_WINDOWS_HPP
#define PAZ_GRAPHICS_WINDOWS_HPP

#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

// Require Windows XP or later.
#if WINVER < 0x0501
#undef WINVER
#define WINVER 0x0501
#endif
#if _WIN32_WINNT < 0x0501
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

// Require DirectInput8.
#define DIRECTINPUT_VERSION 0x0800

#ifndef OEMRESOURCE
#define OEMRESOURCE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif

#include <dinput.h>
#include <dinputd.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <xinput.h>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <windowsx.h>
#include <shellscalingapi.h>
#include <dbt.h>

// Define missing macros.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#endif
#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif
#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif
#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR 0xFFFF
#endif
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef EDS_ROTATEDMODE
#define EDS_ROTATEDMODE 0x00000004
#endif
#ifndef DISPLAY_DEVICE_ACTIVE
#define DISPLAY_DEVICE_ACTIVE 0x00000001
#endif
#ifndef _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT_WINBLUE 0x0603
#endif
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif
#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02e4
#endif
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif
#ifndef OCR_HAND
#define OCR_HAND 32649
#endif

#endif

#endif
