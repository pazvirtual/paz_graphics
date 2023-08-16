#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Engine"
#import <IOKit/hid/IOHIDManager.h>

namespace paz
{
    struct GamepadState
    {
        std::bitset<paz::NumGamepadButtons> buttonDown;
        std::pair<double, double> leftStick;
        std::pair<double, double> rightStick;
        double leftTrigger;
        double rightTrigger;
    };

    struct Gamepad
    {
        IOHIDDeviceRef device;
        GamepadState state;
    };

    extern const char* MacosControllerDb;
}

#endif
