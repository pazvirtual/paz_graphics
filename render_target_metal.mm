#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
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

paz::RenderTarget::RenderTarget()
{
    _data = std::make_unique<Data>();
}

paz::RenderTarget::~RenderTarget()
{
    paz::Window::UnregisterTarget(this);
}

paz::RenderTarget::RenderTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter) :
    RenderTarget()
{
    _scale = scale;
    _data->_numChannels = numChannels;
    _data->_numBits = numBits;
    _data->_type = type;
    Texture::_data->_texture = ::init(_scale*Window::ViewportWidth(), _scale*
        Window::ViewportHeight(), _data->_numChannels, _data->_numBits, _data->
        _type);
    Texture::_data->_sampler = create_sampler(minFilter, magFilter);
    paz::Window::RegisterTarget(this);
}

void paz::RenderTarget::resize(int width, int height)
{
    if(Texture::_data->_texture)
    {
        [(id<MTLTexture>)Texture::_data->_texture release];
    }
    Texture::_data->_texture = ::init(_scale*width, _scale*height, _data->
        _numChannels, _data->_numBits, _data->_type);
}

#endif
