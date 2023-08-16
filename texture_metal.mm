#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

template<typename T, int NumChannels>
static paz::Image<T, NumChannels> flip_image(const paz::Image<T, NumChannels>&
    image)
{
    paz::Image<T, NumChannels> flipped(image.width(), image.height());
    for(int i = 0; i < image.height(); ++i)
    {
        std::copy(image.begin() + image.width()*i*NumChannels, image.begin() +
            (image.width()*i + image.width())*NumChannels, flipped.begin() +
            image.width()*(image.height() - i - 1)*NumChannels);
    }
    return flipped;
}

paz::Texture::Texture()
{
    _data = std::make_unique<Data>();
}

paz::Texture::Texture(const Image<std::uint8_t, 1>& image, MinMagFilter
    minFilter, MinMagFilter magFilter, bool normalized) : Texture()
{
    _width = image.width();
    _height = image.height();
    _data->_format = normalized ? Format::R8UNorm : Format::R8UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    init(flip_image(image).data());
}

paz::Texture::~Texture()
{
    if(_data->_texture)
    {
        [static_cast<id<MTLTexture>>(_data->_texture) release];
    }
    if(_data->_sampler)
    {
        [static_cast<id<MTLSamplerState>>(_data->_sampler) release];
    }
}

paz::Texture::Texture(int width, int height, Format format, MinMagFilter
    minFilter, MinMagFilter magFilter) : Texture()
{
    _width = width;
    _height = height;
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    init();
}

void paz::Texture::init(const void* data)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:pixel_format(_data->_format)];
    [textureDescriptor setWidth:_width];
    [textureDescriptor setHeight:_height];
    if(_data->_isRenderTarget)
    {
        [textureDescriptor setUsage:MTLTextureUsageRenderTarget|
            MTLTextureUsageShaderRead];
        [textureDescriptor setStorageMode:MTLStorageModePrivate];
    }
    else
    {
        [textureDescriptor setUsage:MTLTextureUsageShaderRead];
    }
    _data->_texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
    if(data)
    {
        [static_cast<id<MTLTexture>>(_data->_texture) replaceRegion:
            MTLRegionMake2D(0, 0, _width, _height) mipmapLevel:0 withBytes:data
            bytesPerRow:_width*bytes_per_pixel(_data->_format)];
    }
    if(!_data->_sampler)
    {
        _data->_sampler = create_sampler(_data->_minFilter, _data->_magFilter);
    }
}

void paz::Texture::resize(int width, int height)
{
    if(_data->_scale)
    {
        if(_data->_texture)
        {
            [static_cast<id<MTLTexture>>(_data->_texture) release];
        }
        _width = _data->_scale*width;
        _height = _data->_scale*height;
        init();
    }
}

#endif
