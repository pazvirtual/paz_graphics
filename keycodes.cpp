#include "detect_os.hpp"
#include "keycodes.hpp"

#ifdef PAZ_MACOS

#define CASE(a, b) case 0x##a: return Key::b;

paz::Key paz::convert_keycode(int key) noexcept
{
    switch(key)
    {
        CASE(00, A)
        CASE(01, S)
        CASE(02, D)
        CASE(03, F)
        CASE(04, H)
        CASE(05, G)
        CASE(06, Z)
        CASE(07, X)
        CASE(08, C)
        CASE(09, V)
        CASE(0b, B)
        CASE(0c, Q)
        CASE(0d, W)
        CASE(0e, E)
        CASE(0f, R)
        CASE(10, Y)
        CASE(11, T)
        CASE(12, One)
        CASE(13, Two)
        CASE(14, Three)
        CASE(15, Four)
        CASE(16, Six)
        CASE(17, Five)
        CASE(18, Equal)
        CASE(19, Nine)
        CASE(1a, Seven)
        CASE(1b, Minus)
        CASE(1c, Eight)
        CASE(1d, Zero)
        CASE(1e, RightBracket)
        CASE(1f, O)
        CASE(20, U)
        CASE(21, LeftBracket)
        CASE(22, I)
        CASE(23, P)
        CASE(25, L)
        CASE(26, J)
        CASE(27, Apostrophe)
        CASE(28, K)
        CASE(29, Semicolon)
        CASE(2a, Backslash)
        CASE(2b, Comma)
        CASE(2c, Slash)
        CASE(2d, N)
        CASE(2e, M)
        CASE(2f, Period)
        CASE(32, Grave)
        CASE(41, KeypadDecimal)
        CASE(43, KeypadMultiply)
        CASE(45, KeypadPlus)
        CASE(4b, KeypadDivide)
        CASE(4c, KeypadEnter)
        CASE(4e, KeypadMinus)
        CASE(51, KeypadEqual)
        CASE(52, Keypad0)
        CASE(53, Keypad1)
        CASE(54, Keypad2)
        CASE(55, Keypad3)
        CASE(56, Keypad4)
        CASE(57, Keypad5)
        CASE(58, Keypad6)
        CASE(59, Keypad7)
        CASE(5b, Keypad8)
        CASE(5c, Keypad9)
        CASE(24, Enter)
        CASE(30, Tab)
        CASE(31, Space)
        CASE(33, Backspace)
        CASE(35, Escape)
        CASE(36, RightSuper)
        CASE(37, LeftSuper)
        CASE(38, LeftShift)
        CASE(39, CapsLock)
        CASE(3a, LeftAlt)
        CASE(3b, LeftControl)
        CASE(3c, RightShift)
        CASE(3d, RightAlt)
        CASE(3e, RightControl)
        CASE(40, F17)
        CASE(4f, F18)
        CASE(50, F19)
        CASE(5a, F20)
        CASE(60, F5)
        CASE(61, F6)
        CASE(62, F7)
        CASE(63, F3)
        CASE(64, F8)
        CASE(65, F9)
        CASE(67, F11)
        CASE(69, F13)
        CASE(6a, F16)
        CASE(6b, F14)
        CASE(6d, F10)
        CASE(6f, F12)
        CASE(71, F15)
        CASE(72, Insert)
        CASE(73, Home)
        CASE(74, PageUp)
        CASE(75, Delete)
        CASE(76, F4)
        CASE(77, End)
        CASE(78, F2)
        CASE(79, PageDown)
        CASE(7a, F1)
        CASE(7b, Left)
        CASE(7c, Right)
        CASE(7d, Down)
        CASE(7e, Up)
    }

    return Key::Unknown;
}

#elif defined(PAZ_LINUX)

#include "gl_core_4_1.h"
#include <GLFW/glfw3.h>

#define CASE(a, b) case GLFW_KEY_##a: return Key::b;
#define CASE1(a, b) case GLFW_GAMEPAD_BUTTON_##a: return GamepadButton::b;

paz::Key paz::convert_keycode(int key) noexcept
{
    switch(key)
    {
        CASE(SPACE, Space)
        CASE(APOSTROPHE, Apostrophe)
        CASE(COMMA, Comma)
        CASE(MINUS, Minus)
        CASE(PERIOD, Period)
        CASE(SLASH, Slash)
        CASE(0, Zero)
        CASE(1, One)
        CASE(2, Two)
        CASE(3, Three)
        CASE(4, Four)
        CASE(5, Five)
        CASE(6, Six)
        CASE(7, Seven)
        CASE(8, Eight)
        CASE(9, Nine)
        CASE(SEMICOLON, Semicolon)
        CASE(EQUAL, Equal)
        CASE(A, A)
        CASE(B, B)
        CASE(C, C)
        CASE(D, D)
        CASE(E, E)
        CASE(F, F)
        CASE(G, G)
        CASE(H, H)
        CASE(I, I)
        CASE(J, J)
        CASE(K, K)
        CASE(L, L)
        CASE(M, M)
        CASE(N, N)
        CASE(O, O)
        CASE(P, P)
        CASE(Q, Q)
        CASE(R, R)
        CASE(S, S)
        CASE(T, T)
        CASE(U, U)
        CASE(V, V)
        CASE(W, W)
        CASE(X, X)
        CASE(Y, Y)
        CASE(Z, Z)
        CASE(LEFT_BRACKET, LeftBracket)
        CASE(BACKSLASH, Backslash)
        CASE(RIGHT_BRACKET, RightBracket)
        CASE(GRAVE_ACCENT, Grave)
        CASE(ESCAPE, Escape)
        CASE(ENTER, Enter)
        CASE(TAB, Tab)
        CASE(BACKSPACE, Backspace)
        CASE(INSERT, Insert)
        CASE(DELETE, Delete)
        CASE(RIGHT, Right)
        CASE(LEFT, Left)
        CASE(DOWN, Down)
        CASE(UP, Up)
        CASE(PAGE_UP, PageUp)
        CASE(PAGE_DOWN, PageDown)
        CASE(HOME, Home)
        CASE(END, End)
        CASE(CAPS_LOCK, CapsLock)
        CASE(F1, F1)
        CASE(F2, F2)
        CASE(F3, F3)
        CASE(F4, F4)
        CASE(F5, F5)
        CASE(F6, F6)
        CASE(F7, F7)
        CASE(F8, F8)
        CASE(F9, F9)
        CASE(F10, F10)
        CASE(F11, F11)
        CASE(F12, F12)
        CASE(F13, F13)
        CASE(F14, F14)
        CASE(F15, F15)
        CASE(F16, F16)
        CASE(F17, F17)
        CASE(F18, F18)
        CASE(F19, F19)
        CASE(F20, F20)
        CASE(F21, F21)
        CASE(F22, F22)
        CASE(F23, F23)
        CASE(F24, F24)
        CASE(F25, F25)
        CASE(KP_0, Keypad0)
        CASE(KP_1, Keypad1)
        CASE(KP_2, Keypad2)
        CASE(KP_3, Keypad3)
        CASE(KP_4, Keypad4)
        CASE(KP_5, Keypad5)
        CASE(KP_6, Keypad6)
        CASE(KP_7, Keypad7)
        CASE(KP_8, Keypad8)
        CASE(KP_9, Keypad9)
        CASE(KP_DECIMAL, KeypadDecimal)
        CASE(KP_DIVIDE, KeypadDivide)
        CASE(KP_MULTIPLY, KeypadMultiply)
        CASE(KP_SUBTRACT, KeypadMinus)
        CASE(KP_ADD, KeypadPlus)
        CASE(KP_ENTER, KeypadEnter)
        CASE(KP_EQUAL, KeypadEqual)
        CASE(LEFT_SHIFT, LeftShift)
        CASE(LEFT_CONTROL, LeftControl)
        CASE(LEFT_ALT, LeftAlt)
        CASE(LEFT_SUPER, LeftSuper)
        CASE(RIGHT_SHIFT, RightShift)
        CASE(RIGHT_CONTROL, RightControl)
        CASE(RIGHT_ALT, RightAlt)
        CASE(RIGHT_SUPER, RightSuper)
    }

    return Key::Unknown;
}

paz::GamepadButton paz::convert_button(int button) noexcept
{
    switch(button)
    {
        CASE1(A, A)
        CASE1(B, B)
        CASE1(X, X)
        CASE1(Y, Y)
        CASE1(LEFT_BUMPER, LeftBumper)
        CASE1(RIGHT_BUMPER, RightBumper)
        CASE1(BACK, Back)
        CASE1(START, Start)
        CASE1(GUIDE, Guide)
        CASE1(LEFT_THUMB, LeftThumb)
        CASE1(RIGHT_THUMB, RightThumb)
        CASE1(DPAD_UP, Up)
        CASE1(DPAD_RIGHT, Right)
        CASE1(DPAD_DOWN, Down)
        CASE1(DPAD_LEFT, Left)
    }

    return GamepadButton::Unknown;
}

#else

#define CASE(a, b) case 0x##a: return Key::b;

paz::Key paz::convert_keycode(int key) noexcept
{
    switch(key)
    {
        CASE(00B, Zero)
        CASE(002, One)
        CASE(003, Two)
        CASE(004, Three)
        CASE(005, Four)
        CASE(006, Five)
        CASE(007, Six)
        CASE(008, Seven)
        CASE(009, Eight)
        CASE(00A, Nine)
        CASE(01E, A)
        CASE(030, B)
        CASE(02E, C)
        CASE(020, D)
        CASE(012, E)
        CASE(021, F)
        CASE(022, G)
        CASE(023, H)
        CASE(017, I)
        CASE(024, J)
        CASE(025, K)
        CASE(026, L)
        CASE(032, M)
        CASE(031, N)
        CASE(018, O)
        CASE(019, P)
        CASE(010, Q)
        CASE(013, R)
        CASE(01F, S)
        CASE(014, T)
        CASE(016, U)
        CASE(02F, V)
        CASE(011, W)
        CASE(02D, X)
        CASE(015, Y)
        CASE(02C, Z)
        CASE(028, Apostrophe)
        CASE(02B, Backslash)
        CASE(033, Comma)
        CASE(00D, Equal)
        CASE(029, Grave)
        CASE(01A, LeftBracket)
        CASE(00C, Minus)
        CASE(034, Period)
        CASE(01B, RightBracket)
        CASE(027, Semicolon)
        CASE(035, Slash)
        CASE(00E, Backspace)
        CASE(153, Delete)
        CASE(14F, End)
        CASE(01C, Enter)
        CASE(001, Escape)
        CASE(147, Home)
        CASE(152, Insert)
        CASE(151, PageDown)
        CASE(149, PageUp)
        CASE(039, Space)
        CASE(00F, Tab)
        CASE(03A, CapsLock)
        CASE(03B, F1)
        CASE(03C, F2)
        CASE(03D, F3)
        CASE(03E, F4)
        CASE(03F, F5)
        CASE(040, F6)
        CASE(041, F7)
        CASE(042, F8)
        CASE(043, F9)
        CASE(044, F10)
        CASE(057, F11)
        CASE(058, F12)
        CASE(064, F13)
        CASE(065, F14)
        CASE(066, F15)
        CASE(067, F16)
        CASE(068, F17)
        CASE(069, F18)
        CASE(06A, F19)
        CASE(06B, F20)
        CASE(06C, F21)
        CASE(06D, F22)
        CASE(06E, F23)
        CASE(076, F24)
        CASE(038, LeftAlt)
        CASE(01D, LeftControl)
        CASE(02A, LeftShift)
        CASE(15B, LeftSuper)
        CASE(138, RightAlt)
        CASE(11D, RightControl)
        CASE(036, RightShift)
        CASE(15C, RightSuper)
        CASE(150, Down)
        CASE(14B, Left)
        CASE(14D, Right)
        CASE(148, Up)
        CASE(052, Keypad0)
        CASE(04F, Keypad1)
        CASE(050, Keypad2)
        CASE(051, Keypad3)
        CASE(04B, Keypad4)
        CASE(04C, Keypad5)
        CASE(04D, Keypad6)
        CASE(047, Keypad7)
        CASE(048, Keypad8)
        CASE(049, Keypad9)
        CASE(04E, KeypadPlus)
        CASE(053, KeypadDecimal)
        CASE(135, KeypadDivide)
        CASE(11C, KeypadEnter)
        CASE(059, KeypadEqual)
        CASE(037, KeypadMultiply)
        CASE(04A, KeypadMinus)
    }

    return Key::Unknown;
}

#endif
