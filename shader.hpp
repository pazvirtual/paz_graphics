#ifndef PAZ_GRAPHICS_SHADER_HPP
#define PAZ_GRAPHICS_SHADER_HPP

#include "detect_os.hpp"

#ifndef PAZ_MACOS

#include "PAZ_Graphics"
#include <unordered_map>

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
        // outputTypes[location] = type
        std::unordered_map<unsigned int, unsigned int> _outputTypes;
        void init(unsigned int vertId, unsigned int fragId, const std::
            unordered_map<unsigned int, unsigned int>& outputTypes);
        ShaderData() = default;
        ~ShaderData();
    };
}

#endif

#endif
