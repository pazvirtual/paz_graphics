#ifndef PAZ_GRAPHICS_GAMEPAD_HPP
#define PAZ_GRAPHICS_GAMEPAD_HPP

#include "detect_os.hpp"

#ifndef PAZ_LINUX

#include "PAZ_Graphics"
#ifdef PAZ_MACOS
#include <IOKit/hid/IOHIDManager.h>
#else
#include "windows.hpp"
#endif
#include <bitset>
#include <unordered_map>
#include <cstdint>

namespace paz
{
    struct GamepadElement
    {
        std::size_t idx;
#ifdef PAZ_MACOS
        IOHIDElementRef native;
        std::uint32_t usage;
        long min;
        long max;

        bool operator<(const GamepadElement& elem) const
        {
            return usage < elem.usage || (usage == elem.usage && idx < elem.
                idx);
        }
#else
        bool operator<(const GamepadElement& elem) const
        {
            return idx < elem.idx;
        }
#endif
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
        GUID deviceGuid;
        IDirectInputDevice8* device;
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

#endif
