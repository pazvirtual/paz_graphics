#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.hh"
#import "view_controller.hh"
#include "opengl2metal.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static id<MTLLibrary> create_library(std::string src, bool isVert)
{
    src = (isVert ? paz::vert2metal(src) : paz::frag2metal(src));

    NSError* error = nil;
    id<MTLLibrary> lib = [DEVICE newLibraryWithSource:[NSString
        stringWithUTF8String:src.c_str()] options:nil error:&error];
    if(!lib)
    {
        throw std::runtime_error([[NSString stringWithFormat:@"Failed to create"
            " shader library: %@", [error localizedDescription]] UTF8String]);
    }

    if([[lib functionNames] count] != 1 || ![[lib functionNames][0]
        isEqualToString:(isVert ? @"vertMain" : @"fragMain")])
    {
        throw std::logic_error("Exactly one " + std::string(isVert ? "vertex" :
            "fragment") + " function must be defined in source string.");
    }

    return lib;
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary(const std::vector<std::pair<
    std::string, std::string>>& vertSrcs, const std::vector<std::pair<std::
    string, std::string>>& fragSrcs)
{
    for(const auto& n : vertSrcs)
    {
         if(_verts.count(n.first))
         {
             throw std::runtime_error("Vertex function \"" + n.first + "\" has "
                 "already been defined.");
         }
        _verts[n.first] = create_library(n.second, true);
    }
    for(const auto& n : fragSrcs)
    {
         if(_frags.count(n.first))
         {
             throw std::runtime_error("Fragment function \"" + n.first + "\" ha"
                 "s already been defined.");
         }
        _frags[n.first] = create_library(n.second, false);
    }
}

paz::ShaderFunctionLibrary::~ShaderFunctionLibrary()
{
    for(const auto& n : _verts)
    {
        if(n.second)
        {
            [(id<MTLLibrary>)n.second release];
        }
    }
    for(const auto& n : _frags)
    {
        if(n.second)
        {
            [(id<MTLLibrary>)n.second release];
        }
    }
}

#endif
