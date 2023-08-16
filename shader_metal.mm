#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

paz::Shader::Data::~Data()
{
    if(_vert)
    {
        [static_cast<id<MTLFunction>>(_vert) release];
    }
    if(_frag)
    {
        [static_cast<id<MTLFunction>>(_frag) release];
    }
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    initialize();

    _data = std::make_shared<Data>();

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

    _data->_vert = [static_cast<id<MTLLibrary>>(vertLibrary._data->_verts.at(
        vertName)) newFunctionWithName:@"vertMain"];
    _data->_frag = [static_cast<id<MTLLibrary>>(fragLibrary._data->_frags.at(
        fragName)) newFunctionWithName:@"fragMain"];
}

paz::Shader::Shader(const ShaderFunctionLibrary& /* vertLibrary */, const std::
    string& /* vertName */, const ShaderFunctionLibrary& /* geomLibrary */,
    const std::string& /* geomName */, const ShaderFunctionLibrary& /*
    fragLibrary */, const std::string& /* fragName */)
{
    throw std::logic_error("NOT IMPLEMENTED");
}

#endif
