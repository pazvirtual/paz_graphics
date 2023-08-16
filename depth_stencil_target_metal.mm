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

static MTLPixelFormat depth_pixel_format(unsigned int b, paz::Texture::DataType
    t)
{
    if(b == 16)
    {
        if(t == paz::Texture::DataType::UNorm)
        {
            return MTLPixelFormatDepth16Unorm;
        }
    }
    else if(b == 32)
    {
        if(t == paz::Texture::DataType::Float)
        {
            return MTLPixelFormatDepth32Float;
        }
    }

    throw std::runtime_error("Invalid depth texture format requested.");
}

static id<MTLTexture> init(int width, int height, int numBits, paz::Texture::
    DataType type)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:depth_pixel_format(numBits, type)];
    [textureDescriptor setWidth:width];
    [textureDescriptor setHeight:height];
    [textureDescriptor setUsage:MTLTextureUsageRenderTarget|
        MTLTextureUsageShaderRead];
    [textureDescriptor setStorageMode:MTLStorageModePrivate];
    id<MTLTexture> texture = [DEVICE newTextureWithDescriptor:
        textureDescriptor];
    [textureDescriptor release];
    return texture;
}

paz::DepthStencilTarget::~DepthStencilTarget()
{
    unregister_target(this);
}

paz::DepthStencilTarget::DepthStencilTarget(double scale, int numBits, DataType
    type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    _data->_numChannels = 1;
    _data->_numBits = numBits;
    _data->_type = type;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_texture = ::init(_scale*Window::ViewportWidth(), _scale*Window::
        ViewportHeight(), _data->_numBits, _data->_type);
    _data->_sampler = create_sampler(_data->_minFilter, _data->_magFilter);
    register_target(this);
}

void paz::DepthStencilTarget::resize(int width, int height)
{
    if(_data->_texture)
    {
        [(id<MTLTexture>)_data->_texture release];
    }
    _data->_texture = ::init(_scale*width, _scale*height, _data->_numBits,
        _data->_type);
}

#endif
