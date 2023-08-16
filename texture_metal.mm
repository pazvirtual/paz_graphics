#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::Texture::Texture() {}

paz::Texture::~Texture()
{
    if(_texture)
    {
        [(id<MTLTexture>)_texture release];
    }
    if(_sampler)
    {
        [(id<MTLSamplerState>)_sampler release];
    }
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    init(width, height, numChannels, numBits, type, minFilter, magFilter,
        nullptr);
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits, const
    std::vector<float>& data, MinMagFilter minFilter, MinMagFilter magFilter)
{
    std::vector<float> v(data.size());
    for(int i = 0; i < height; ++i)
    {
        std::copy(data.begin() + width*i, data.begin() + width*i + width, v.
            begin() + width*(height - i - 1));
    }
    init(width, height, numChannels, numBits, DataType::Float, minFilter,
        magFilter, v.data());
}

// ...

void paz::Texture::init(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, const void*
    data)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:pixel_format(numChannels, numBits, type)];
    [textureDescriptor setWidth:width];
    [textureDescriptor setHeight:height];
    [textureDescriptor setUsage:MTLTextureUsageShaderRead];
    _texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
    if(data)
    {
        [(id<MTLTexture>)_texture replaceRegion:MTLRegionMake2D(0, 0, width,
            height) mipmapLevel:0 withBytes:data bytesPerRow:width*numChannels*
            numBits/8];
    }
    if(!_sampler)
    {
        _sampler = create_sampler(minFilter, magFilter);
    }
}

void paz::Texture::resize(int width, int height)
{
    throw std::runtime_error("TEXTURE RESIZE NOT IMPLEMENTED");
}

#endif
