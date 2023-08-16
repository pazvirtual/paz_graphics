#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#include "window.hpp"
#import <MetalKit/MetalKit.h>

#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])
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

paz::Texture::Data::~Data()
{
    if(_isRenderTarget)
    {
        unregister_target(this);
    }

    if(_texture)
    {
        [static_cast<id<MTLTexture>>(_texture) release];
    }
    if(_sampler)
    {
        [static_cast<id<MTLSamplerState>>(_sampler) release];
    }
}

paz::Texture::Texture() {}

paz::Texture::Texture(const Image<std::uint8_t, 1>& image, MinMagFilter
    minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode wrapT,
    MipmapFilter mipFilter, bool normalized)
{
    initialize();

    if(image.width()%4)
    {
        throw std::runtime_error("Image width must be a multiple of four.");
    }

    _data = std::make_shared<Data>();
    _data->_width = image.width();
    _data->_height = image.height();
    _data->_format = normalized ? TextureFormat::R8UNorm : TextureFormat::
        R8UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->init(flip_image(image).data());
}

paz::Texture::Texture(int width, int height, TextureFormat format, MinMagFilter
    minFilter, MinMagFilter magFilter, WrapMode wrapS, WrapMode wrapT,
    MipmapFilter mipFilter)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = width;
    _data->_height = height;
    _data->_format = format;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->init();
}

void paz::Texture::Data::init(const void* data)
{
    MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:pixel_format(_format) width:_width
        height:_height mipmapped:(_mipFilter == MipmapFilter::None ? NO : YES)];
    [textureDescriptor setUsage:(_isRenderTarget ? MTLTextureUsageRenderTarget|
        MTLTextureUsageShaderRead : MTLTextureUsageShaderRead)];
    if(_format == TextureFormat::Depth16UNorm || _format == TextureFormat::
        Depth32Float)
    {
        [textureDescriptor setStorageMode:MTLStorageModePrivate];
    }
    _texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
    if(data)
    {
        [static_cast<id<MTLTexture>>(_texture) replaceRegion:MTLRegionMake2D(0,
            0, _width, _height) mipmapLevel:0 withBytes:data bytesPerRow:_width*
            bytes_per_pixel(_format)];
    }
    if(!_sampler)
    {
        _sampler = create_sampler(_minFilter, _magFilter, _wrapS, _wrapT);
    }
    ensureMipmaps();
}

void paz::Texture::Data::resize(int width, int height)
{
    if(_scale)
    {
        if(_texture)
        {
            [static_cast<id<MTLTexture>>(_texture) release];
        }
        _width = _scale*width;
        _height = _scale*height;
        init();
    }
}

void paz::Texture::Data::ensureMipmaps()
{
    if(_mipFilter != MipmapFilter::None)
    {
        id<MTLBlitCommandEncoder> blitEncoder = [[RENDERER commandBuffer]
            blitCommandEncoder];
        [blitEncoder generateMipmapsForTexture:static_cast<id<MTLTexture>>(
            _texture)];
        [blitEncoder endEncoding];
    }
}

int paz::Texture::width() const
{
    return _data->_width;
}

int paz::Texture::height() const
{
    return _data->_height;
}

#endif
