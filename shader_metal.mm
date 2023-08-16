#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

paz::Shader::~Shader()
{
    if(_data->_vert)
    {
        [(id<MTLFunction>)_data->_vert release];
    }
    if(_data->_frag)
    {
        [(id<MTLFunction>)_data->_frag release];
    }
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    _data = std::make_unique<Data>();

    if(!vertLibrary._data->_verts.count(vertName))
    {
        throw std::runtime_error("Vertex function \"" + vertName + "\" not foun"
            "d in library.");
    }

    if(!fragLibrary._data->_frags.count(fragName))
    {
        throw std::runtime_error("Fragment function \"" + fragName + "\" not fo"
            "und in library.");
    }

    _data->_vert = [(id<MTLLibrary>)vertLibrary._data->_verts.at(vertName)
        newFunctionWithName:@"vertMain"];
    _data->_frag = [(id<MTLLibrary>)fragLibrary._data->_frags.at(fragName)
        newFunctionWithName:@"fragMain"];
}

#endif
