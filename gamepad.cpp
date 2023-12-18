#include "detect_os.hpp"

#ifndef PAZ_LINUX

#include "gamepad.hpp"
#include "controller_db.hpp"
#include <sstream>

#ifdef PAZ_MACOS
static const std::string Platform = "Mac OS X";
#elif defined(PAZ_WINDOWS)
static const std::string Platform = "Windows";
#endif

#define CASE0(a, b) {#a, paz::GamepadButton::b},
#define CASE1(a, b) {#a, b},

static bool parse_mapping(const std::string& str, paz::GamepadMapping& m)
{
    static const std::unordered_map<std::string, int> axisIndices =
    {
        CASE1(leftx, 0)
        CASE1(lefty, 1)
        CASE1(rightx, 2)
        CASE1(righty, 3)
        CASE1(lefttrigger, 4)
        CASE1(righttrigger, 5)
    };

    static const std::unordered_map<std::string, paz::GamepadButton>
        buttonNames =
    {
        CASE0(a, A)
        CASE0(b, B)
        CASE0(x, X)
        CASE0(y, Y)
        CASE0(back, Back)
        CASE0(start, Start)
        CASE0(guide, Guide)
        CASE0(leftshoulder, LeftBumper)
        CASE0(rightshoulder, RightBumper)
        CASE0(leftstick, LeftThumb)
        CASE0(rightstick, RightThumb)
    };

    static const std::unordered_map<std::string, paz::GamepadButton> hatNames =
    {
        CASE0(dpup, Up)
        CASE0(dpright, Right)
        CASE0(dpdown, Down)
        CASE0(dpleft, Left)
    };

    // Check platform.
    if(str.substr(str.find("platform:") + 9, Platform.size()) != Platform)
    {
        return false;
    }

    std::stringstream ss(str);
    std::string line;

    // Skip name.
    std::getline(ss, line, ',');

    // Get mapping.
    while(std::getline(ss, line, ','))
    {
        // GLFW does not currently support input modifiers, so neither will we.
        if(line.empty() || line[0] == '-' || line[0] == '+')
        {
            return false;
        }

        const std::size_t splitPos = line.find(':');
        if(splitPos == std::string::npos)
        {
            return false;
        }
        const std::string key = line.substr(0, splitPos);
        const std::string val = line.substr(splitPos + 1);
        if(val.size() < 2)
        {
            return false;
        }

        int min = -1;
        int max = 1;
        std::size_t startPos = 0;
        if(val[0] == '-')
        {
            min = 0;
            ++startPos;
        }
        if(val[0] == '+')
        {
            max = 0;
            ++startPos;
        }
        if(val[startPos] == 'a')
        {
            if(axisIndices.count(key))
            {
                const std::size_t endPos = val.find('~');
                const int idx = axisIndices.at(key);
                m.axes[idx].type = paz::GamepadElementType::Axis;
                m.axes[idx].idx = std::stoi(val.substr(startPos + 1, endPos));
                if(max == min)
                {
                    return false;
                }
                m.axes[idx].axisScale = 2/(max - min);
                m.axes[idx].axisOffset = -(max + min);
                if(endPos != std::string::npos)
                {
                    m.axes[idx].axisScale = -m.axes[idx].axisScale;
                    m.axes[idx].axisOffset = -m.axes[idx].axisOffset;
                }
            }
        }
        else if(val[startPos] == 'b')
        {
            if(buttonNames.count(key))
            {
                const int idx = static_cast<int>(buttonNames.at(key));
                m.buttons[idx].type = paz::GamepadElementType::Button;
                m.buttons[idx].idx = std::stoi(val.substr(startPos + 1));
            }
        }
        else if(val[startPos] == 'h')
        {
            if(hatNames.count(key))
            {
                const int idx = static_cast<int>(hatNames.at(key));
                m.buttons[idx].type = paz::GamepadElementType::HatBit;
                std::size_t pos = val.find('.');
                if(pos == std::string::npos)
                {
                    return false;
                }
                const unsigned long hat = std::stoi(val.substr(startPos + 1, pos
                    - 1));
                const unsigned long bit = std::stoi(val.substr(pos + 1));
                m.buttons[idx].idx = static_cast<std::uint8_t>((hat << 4)|bit);
            }
        }
    }

    return true;
}

const std::unordered_map<std::string, paz::GamepadMapping>& paz::
    gamepad_mappings()
{
    static const auto res = []()
    {
        std::unordered_map<std::string, GamepadMapping> mappings;
        std::stringstream iss(ControllerDb);
        std::string line;
        while(std::getline(iss, line))
        {
            if(line.empty() || line[0] == '#')
            {
                continue;
            }
            const std::string guid = line.substr(0, 32);
            if(!mappings.count(guid))
            {
                GamepadMapping m;
                if(parse_mapping(line.substr(33), m))
                {
                    mappings[guid] = m;
                }
            }
        }
        return mappings;
    }();
    return res;
}

#endif
