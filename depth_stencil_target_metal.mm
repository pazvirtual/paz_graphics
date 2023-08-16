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

paz::DepthStencilTarget::DepthStencilTarget(double scale, int numBits, DataType
    type, MinMagFilter minFilter, MinMagFilter magFilter) : RenderTarget()
{
    _scale = scale;
    _data->_numChannels = 1;
    _data->_numBits = numBits;
    _data->_type = type;
    Texture::_data->_texture = ::init(_scale*Window::ViewportWidth(), _scale*
        Window::ViewportHeight(), _data->_numBits, _data->_type);
    Texture::_data->_sampler = create_sampler(minFilter, magFilter);
    paz::Window::RegisterTarget(this);
}

void paz::DepthStencilTarget::resize(int width, int height)
{
    if(Texture::_data->_texture)
    {
        [(id<MTLTexture>)Texture::_data->_texture release];
    }
    Texture::_data->_texture = ::init(_scale*width, _scale*height, _data->
        _numBits, _data->_type);
}

#endif
