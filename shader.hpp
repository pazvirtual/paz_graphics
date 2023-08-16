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
        unsigned int _thickLinesId = 0;
        // uniformIDs[name] = (id, type, size)
        std::unordered_map<std::string, std::tuple<unsigned int, unsigned int,
            int>> _uniformIds;
        std::unordered_map<std::string, std::tuple<unsigned int, unsigned int,
            int>> _thickLinesUniformIds;
        // attribTypes[location] = type (array attributes are not supported)
        std::unordered_map<unsigned int, unsigned int> _attribTypes;
        void init(unsigned int vertId, unsigned int thickLinesVertId, unsigned
            int thickLinesGeomId, unsigned int fragId);
        ShaderData();
        ~ShaderData();
    };
}

#endif

#endif
