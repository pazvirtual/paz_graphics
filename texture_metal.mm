#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "util_metal.h"
#import "app_delegate.h"
#import "view_controller.h"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static MTLSamplerMinMagFilter min_mag_filter(paz::TextureBase::MinMagFilter f)
{
    switch(f)
    {
        case paz::TextureBase::MinMagFilter::Linear: return
            MTLSamplerMinMagFilterLinear;
        case paz::TextureBase::MinMagFilter::Nearest: return
            MTLSamplerMinMagFilterNearest;
    }

    throw std::logic_error("Invalid texture filter requested.");
}

void paz::Texture::createSampler(MinMagFilter minFilter, MinMagFilter magFilter,
    bool repeat)
{
    MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
    [descriptor setMinFilter:min_mag_filter(minFilter)];
    [descriptor setMagFilter:min_mag_filter(magFilter)];
    [descriptor setSAddressMode:(repeat ? MTLSamplerAddressModeRepeat :
        MTLSamplerAddressModeClampToEdge)];
    [descriptor setTAddressMode:(repeat ? MTLSamplerAddressModeRepeat :
        MTLSamplerAddressModeClampToEdge)];
    _sampler = [DEVICE newSamplerStateWithDescriptor:descriptor];
    [descriptor release];
}

paz::Texture::Texture() {}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, bool repeat,
    const void* data)
{
    init(width, height, numChannels, numBits, type, minFilter, magFilter,
        repeat, data);
}

void paz::Texture::init(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter, bool repeat,
    const void* data)
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
        createSampler(minFilter, magFilter, repeat);
    }
}

void paz::Texture::resize(int width, int height)
{
    throw std::runtime_error("TEXTURE RESIZE NOT IMPLEMENTED");
}

#endif
