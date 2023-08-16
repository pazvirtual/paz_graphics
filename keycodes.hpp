#ifndef PAZ_GRAPHICS_KEYCODES_HPP
#define PAZ_GRAPHICS_KEYCODES_HPP

#include "PAZ_Graphics"
#include "detect_os.hpp"

namespace paz
{
    paz::Key convert_keycode(int key) noexcept;
#ifdef PAZ_LINUX
    paz::GamepadButton convert_button(int button) noexcept;
#endif
}

#endif
