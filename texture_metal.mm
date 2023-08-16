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

paz::Texture::Texture()
{
    initialize();
}

#define TEX(t, n, f) paz::Texture::Texture(const Image<t, n>& image, \
    MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, \
    WrapMode wrapS, WrapMode wrapT)\
{\
    initialize();\
    if(image.width()%4)\
    {\
        throw std::runtime_error("Image width must be a multiple of four.");\
    }\
    _data = std::make_shared<Data>();\
    _data->_width = image.width();\
    _data->_height = image.height();\
    _data->_format = TextureFormat::f;\
    _data->_minFilter = minFilter;\
    _data->_magFilter = magFilter;\
    _data->_mipFilter = mipFilter;\
    _data->_wrapS = wrapS;\
    _data->_wrapT = wrapT;\
    _data->init(flip_image(image).data());\
}

#define TEX_NORM(t, n, f) paz::Texture::Texture(const Image<t, n>& image, \
    MinMagFilter minFilter, MinMagFilter magFilter, MipmapFilter mipFilter, \
    WrapMode wrapS, WrapMode wrapT, bool normalized)\
{\
    initialize();\
    if(image.width()%4)\
    {\
        throw std::runtime_error("Image width must be a multiple of four.");\
    }\
    _data = std::make_shared<Data>();\
    _data->_width = image.width();\
    _data->_height = image.height();\
    _data->_format = normalized ? TextureFormat::f##Norm : TextureFormat::\
        f##Int;\
    _data->_minFilter = minFilter;\
    _data->_magFilter = magFilter;\
    _data->_mipFilter = mipFilter;\
    _data->_wrapS = wrapS;\
    _data->_wrapT = wrapT;\
    _data->init(flip_image(image).data());\
}

TEX_NORM(std::int8_t, 1, R8S)
TEX_NORM(std::int8_t, 2, RG8S)
TEX_NORM(std::int8_t, 4, RGBA8S)
TEX_NORM(std::int16_t, 1, R16S)
TEX_NORM(std::int16_t, 2, RG16S)
TEX_NORM(std::int16_t, 4, RGBA16S)
TEX(std::int32_t, 1, R32SInt)
TEX(std::int32_t, 2, RG32SInt)
TEX(std::int32_t, 4, RGBA32SInt)
TEX_NORM(std::uint8_t, 1, R8U)
TEX_NORM(std::uint8_t, 2, RG8U)
TEX_NORM(std::uint8_t, 4, RGBA8U)
TEX_NORM(std::uint16_t, 1, R16U)
TEX_NORM(std::uint16_t, 2, RG16U)
TEX_NORM(std::uint16_t, 4, RGBA16U)
TEX(std::uint32_t, 1, R32UInt)
TEX(std::uint32_t, 2, RG32UInt)
TEX(std::uint32_t, 4, RGBA32UInt)
TEX(float, 1, R32Float)
TEX(float, 2, RG32Float)
TEX(float, 4, RGBA32Float)

paz::Texture::Texture(int width, int height, TextureFormat format, MinMagFilter
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

void paz::Texture::Data::init(const void* data)
{
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
    [textureDescriptor release];
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
