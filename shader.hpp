#ifndef PAZ_GRAPHICS_SHADER_HPP
#define PAZ_GRAPHICS_SHADER_HPP

#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"

namespace paz
{
    struct ShaderData
    {
        unsigned int _id = 0;
        // uniformIDs[name] = (id, type, size)
        std::unordered_map<std::string, std::tuple<unsigned int, unsigned int,
            int>> _uniformIds;
        // attribTypes[location] = type (array attributes are not supported)
        std::unordered_map<unsigned int, unsigned int> _attribTypes;
        bool _thickLines = false;
        void init(unsigned int vertId, unsigned int thickLinesId, unsigned int
            fragId);
        ShaderData();
        ~ShaderData();
    };
}

#endif

#endif
