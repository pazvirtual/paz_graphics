#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import <MetalKit/MetalKit.h>

paz::Shader::~Shader()
{
    if(_vert)
    {
        [(id<MTLFunction>)_vert release];
    }
    if(_frag)
    {
        [(id<MTLFunction>)_frag release];
    }
}

paz::Shader::Shader(const ShaderFunctionLibrary& vertLibrary, const std::string&
    vertName, const ShaderFunctionLibrary& fragLibrary, const std::string&
    fragName)
{
    _vert = [(id<MTLLibrary>)vertLibrary._verts.at(vertName)
        newFunctionWithName:@"vertMain"];
    _frag = [(id<MTLLibrary>)fragLibrary._frags.at(fragName)
        newFunctionWithName:@"fragMain"];
}

#endif
