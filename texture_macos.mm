#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_macos.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#include "common.hpp"
#import <MetalKit/MetalKit.h>

#define RENDERER static_cast<Renderer*>([static_cast<ViewController*>( \
    [[static_cast<AppDelegate*>([NSApp delegate]) window] \
    contentViewController]) renderer])
#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

#define CASE(a, b) case paz::WrapMode::a: return MTLSamplerAddressMode##b;

static MTLSamplerMinMagFilter min_mag_filter(paz::MinMagFilter f)
{
    switch(f)
    {
        case paz::MinMagFilter::Linear: return MTLSamplerMinMagFilterLinear;
        case paz::MinMagFilter::Nearest: return MTLSamplerMinMagFilterNearest;
    }

    throw std::logic_error("Invalid texture filter requested.");
}

static MTLSamplerAddressMode address_mode(paz::WrapMode m)
{
    switch(m)
    {
        CASE(Repeat, Repeat)
        CASE(MirrorRepeat, MirrorRepeat)
        CASE(ClampToEdge, ClampToEdge)
        CASE(ClampToZero, ClampToZero)
    }

    throw std::logic_error("Invalid texture wrapping mode requested.");
}

static id<MTLSamplerState> create_sampler(paz::MinMagFilter minFilter, paz::
    MinMagFilter magFilter, paz::MipmapFilter mipFilter, paz::WrapMode wrapS,
    paz::WrapMode wrapT)
{
    MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
    [descriptor setMinFilter:min_mag_filter(minFilter)];
    [descriptor setMagFilter:min_mag_filter(magFilter)];
    if(mipFilter == paz::MipmapFilter::Linear)
    {
        [descriptor setMipFilter:MTLSamplerMipFilterLinear];
    }
    else if(mipFilter == paz::MipmapFilter::Nearest)
    {
        [descriptor setMipFilter:MTLSamplerMipFilterNearest];
    }
    [descriptor setSAddressMode:address_mode(wrapS)];
    [descriptor setTAddressMode:address_mode(wrapT)];
    id<MTLSamplerState> sampler = [DEVICE newSamplerStateWithDescriptor:
        descriptor];
    [descriptor release];
    return sampler;
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

paz::Texture::Texture()
{
    initialize();
}

paz::Texture::Texture(TextureFormat format, int width, int height, MinMagFilter
    minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, WrapMode wrapS,
    WrapMode wrapT)
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

paz::Texture::Texture(TextureFormat format, int width, int height, const void*
    data, MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter
    mipFilter, WrapMode wrapS, WrapMode wrapT)
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
    if(height < 2)
    {
        _data->init(data);
    }
    else
    {
        const std::size_t bytesPerRow = paz::bytes_per_pixel(format)*width;
        std::vector<unsigned char> flipped(bytesPerRow*height);
        for(int i = 0; i < height; ++i)
        {
            std::copy(reinterpret_cast<const unsigned char*>(data) +
                bytesPerRow*i, reinterpret_cast<const unsigned char*>(data) +
                bytesPerRow*(i + 1), flipped.begin() + bytesPerRow*(height - i -
                1));
        }
        _data->init(flipped.data());
    }
}

paz::Texture::Texture(const Image& image, MinMagFilter minFilter, MinMagFilter
    magFilter, MipmapFilter mipFilter, WrapMode wrapS, WrapMode wrapT)
{
    initialize();

    _data = std::make_shared<Data>();
    _data->_width = image.width();
    _data->_height = image.height();
    _data->_format = static_cast<TextureFormat>(image.format());
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    _data->_mipFilter = mipFilter;
    _data->_wrapS = wrapS;
    _data->_wrapT = wrapT;
    _data->init(flip_image(image).bytes().data());
}

paz::Texture::Texture(RenderTarget&& target) : _data(std::move(target._data)) {}

void paz::Texture::Data::init(const void* data)
{
    // Textures not for rendering are static (for now).
    if(!_isRenderTarget && !data)
    {
        throw std::logic_error("Cannot initialize static texture without data."
            );
    }

    // This is because of Metal restrictions.
    if((_format == TextureFormat::Depth16UNorm || _format == TextureFormat::
        Depth32Float) && _mipFilter != MipmapFilter::None)
    {
        throw std::runtime_error("Depth/stencil textures do not support mipmapp"
            "ing.");
    }

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
    if(data)
    {
        [static_cast<id<MTLTexture>>(_texture) replaceRegion:MTLRegionMake2D(0,
            0, _width, _height) mipmapLevel:0 withBytes:data bytesPerRow:_width*
            bytes_per_pixel(_format)];
    }
    if(!_sampler)
    {
        _sampler = create_sampler(_minFilter, _magFilter, _mipFilter, _wrapS,
            _wrapT);
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
        [RENDERER ensureCommandBuffer];
        id<MTLBlitCommandEncoder> blitEncoder = [[RENDERER commandBuffer]
            blitCommandEncoder];
        [blitEncoder generateMipmapsForTexture:static_cast<id<MTLTexture>>(
            _texture)];
        [blitEncoder endEncoding];
    }
}

int paz::Texture::width() const
{
    return _data ? _data->_width : 0;
}

int paz::Texture::height() const
{
    return _data ? _data->_height : 0;
}

#endif
