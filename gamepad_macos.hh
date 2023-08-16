#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import <IOKit/hid/IOHIDManager.h>

namespace paz
{
    struct GamepadElement
    {
        IOHIDElementRef native;
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
        IOHIDDeviceRef device;
        std::string guid;
        std::string name;
        std::vector<GamepadElement> axes;
        std::vector<GamepadElement> buttons;
        std::vector<GamepadElement> hats;
    };

    extern const char* MacosControllerDb;
}

#endif
