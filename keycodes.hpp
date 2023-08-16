#ifndef PAZ_GRAPHICS_KEYCODES_HPP
#define PAZ_GRAPHICS_KEYCODES_HPP

#include "PAZ_Graphics"

namespace paz
{
    paz::Key convert_keycode(int key) noexcept;
    paz::GamepadButton convert_button(int button) noexcept;
}

#endif
