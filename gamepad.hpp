#include "detect_os.hpp"

#ifndef PAZ_LINUX

#include "PAZ_Graphics"
#ifdef PAZ_MACOS
#include <IOKit/hid/IOHIDManager.h>
#else
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
#endif
#include <bitset>
#include <unordered_map>
#include <cstdint>

namespace paz
{
    struct GamepadElement
    {
#ifdef PAZ_MACOS
        IOHIDElementRef native;
#endif
        std::uint32_t usage;
        std::size_t idx;
        long min;
        long max;

        bool operator<(const GamepadElement& elem) const
        {
            return usage < elem.usage || (usage == elem.usage && idx < elem.
                idx);
        }
    };

    struct GamepadState
    {
        std::bitset<paz::NumGamepadButtons> buttons;
        std::array<double, 6> axes;
    };

    enum class GamepadElementType
    {
        Axis, HatBit, Button
    };

    struct GamepadMapElement
    {
        GamepadElementType type;
        int idx;
        int axisScale;
        int axisOffset;
    };

    struct GamepadMapping
    {
        std::array<GamepadMapElement, paz::NumGamepadButtons> buttons;
        std::array<GamepadMapElement, 6> axes;
    };

    struct Gamepad
    {
#ifdef PAZ_MACOS
        IOHIDDeviceRef device;
#else
        IDirectInputDevice8W* device;
#endif
        std::string guid;
        std::string name;
        std::vector<GamepadElement> axes;
        std::vector<GamepadElement> buttons;
        std::vector<GamepadElement> hats;
    };

    const std::unordered_map<std::string, GamepadMapping>& gamepad_mappings();
}

#endif
