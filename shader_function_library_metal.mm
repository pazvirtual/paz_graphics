#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "opengl2metal.hpp"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

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

paz::ShaderFunctionLibrary::Data::~Data()
{
    for(const auto& n : _verts)
    {
        if(n.second)
        {
            [static_cast<id<MTLLibrary>>(n.second) release];
        }
    }
    for(const auto& n : _frags)
    {
        if(n.second)
        {
            [static_cast<id<MTLLibrary>>(n.second) release];
        }
    }
}

paz::ShaderFunctionLibrary::ShaderFunctionLibrary()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::ShaderFunctionLibrary::vertex(const std::string& name, const std::
    string& src)
{
    if(_data->_verts.count(name))
    {
        throw std::runtime_error("Vertex function \"" + name + "\" has already "
            "been defined.");
    }

    try
    {
        _data->_verts[name] = create_library(src, true);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile vertex function \"" + name +
            "\": " + e.what());
    }
}

void paz::ShaderFunctionLibrary::geometry(const std::string& /* name */, const
    std::string& /* src */)
{
    throw std::logic_error("NOT IMPLEMENTED");
}

void paz::ShaderFunctionLibrary::fragment(const std::string& name, const std::
    string& src)
{
    if(_data->_frags.count(name))
    {
        throw std::runtime_error("Fragment function \"" + name + "\" has alread"
            "y been defined.");
    }

    try
    {
        _data->_frags[name] = create_library(src, false);
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("Failed to compile fragment function \"" + name
            + "\": " + e.what());
    }
}

#endif
