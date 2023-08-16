#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "shading_lang.hpp"
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

paz::VertexFunction::Data::~Data()
{
    [static_cast<id<MTLFunction>>(_function) release];
}

paz::FragmentFunction::Data::~Data()
{
    [static_cast<id<MTLFunction>>(_function) release];
}

paz::VertexFunction::VertexFunction()
{
    initialize();

    _data = std::make_shared<Data>();
};

paz::FragmentFunction::FragmentFunction()
{
    initialize();

    _data = std::make_shared<Data>();
};

paz::VertexFunction::VertexFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();

    id<MTLLibrary> lib = create_library(src, true);
    _data->_function = [lib newFunctionWithName:@"vertMain"];
    [lib release];
}

paz::FragmentFunction::FragmentFunction(const std::string& src)
{
    initialize();

    _data = std::make_shared<Data>();

    id<MTLLibrary> lib = create_library(src, false);
    _data->_function = [lib newFunctionWithName:@"fragMain"];
    [lib release];
}

#endif
