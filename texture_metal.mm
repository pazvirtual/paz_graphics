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

paz::Texture::Texture()
{
    _data = std::make_unique<Data>();
}

paz::Texture::~Texture()
{
    if(_data->_texture)
    {
        [(id<MTLTexture>)_data->_texture release];
    }
    if(_data->_sampler)
    {
        [(id<MTLSamplerState>)_data->_sampler release];
    }
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter) :
    Texture()
{
    _data->_numChannels = numChannels;
    _data->_numBits = numBits;
    _data->_type = type;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    init(width, height, _data->_numChannels, _data->_numBits, _data->_type,
        _data->_minFilter, _data->_magFilter, nullptr);
}

paz::Texture::Texture(int width, int height, int numChannels, int numBits, const
    std::vector<float>& data, MinMagFilter minFilter, MinMagFilter magFilter) :
    Texture()
{
    _data->_numChannels = numChannels;
    _data->_numBits = numBits;
    _data->_type = DataType::Float;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    std::vector<float> v(data.size());
    for(int i = 0; i < height; ++i)
    {
        std::copy(data.begin() + width*i, data.begin() + width*i + width, v.
            begin() + width*(height - i - 1));
    }
    init(width, height, _data->_numChannels, _data->_numBits, _data->_type,
        _data->_minFilter, _data->_magFilter, v.data());
}

// ...

paz::Texture::Texture(int width, int height, int numChannels, int numBits, const
    std::vector<unsigned int>& data, MinMagFilter minFilter, MinMagFilter
    magFilter) : Texture()
{
    _data->_numChannels = numChannels;
    _data->_numBits = numBits;
    _data->_type = DataType::UInt;
    _data->_minFilter = minFilter;
    _data->_magFilter = magFilter;
    std::vector<unsigned int> v(data.size());
    for(int i = 0; i < height; ++i)
    {
        std::copy(data.begin() + width*i, data.begin() + width*i + width, v.
            begin() + width*(height - i - 1));
    }
    init(width, height, _data->_numChannels, _data->_numBits, _data->_type,
        _data->_minFilter, _data->_magFilter, v.data());
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
    _data->_texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
    if(data)
    {
        [(id<MTLTexture>)_data->_texture replaceRegion:MTLRegionMake2D(0, 0,
            width, height) mipmapLevel:0 withBytes:data bytesPerRow:width*
            numChannels*numBits/8];
    }
    if(!_data->_sampler)
    {
        _data->_sampler = create_sampler(minFilter, magFilter);
    }
}

void paz::Texture::resize(int width, int height)
{
    if(_data->_texture)
    {
        [(id<MTLTexture>)_data->_texture release];
    }
    init(width, height, _data->_numChannels, _data->_numBits, _data->_type,
        _data->_minFilter, _data->_magFilter, nullptr);
}

#endif
