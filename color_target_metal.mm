#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static id<MTLTexture> init(int width, int height, int numChannels, int numBits,
    paz::Texture::DataType type)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:pixel_format(numChannels, numBits, type)];
    [textureDescriptor setWidth:width];
    [textureDescriptor setHeight:height];
    [textureDescriptor setUsage:MTLTextureUsageRenderTarget|
        MTLTextureUsageShaderRead];
    id<MTLTexture> texture = [DEVICE newTextureWithDescriptor:
        textureDescriptor];
    [textureDescriptor release];
    return texture;
}

paz::ColorTarget::~ColorTarget()
{
    unregister_target(this);
}

paz::ColorTarget::ColorTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    _width = _scale*Window::ViewportWidth();
    _height = _scale*Window::ViewportHeight();
    _data->_numChannels = numChannels;
    _data->_numBits = numBits;
    _data->_type = type;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_texture = ::init(_width, _height, _data->_numChannels, _data->
        _numBits, _data->_type);
    _data->_sampler = create_sampler(minFilter, magFilter);
    register_target(this);
}

void paz::ColorTarget::resize(int width, int height)
{
    Texture::resize(_scale*width, _scale*height);
}

#endif
