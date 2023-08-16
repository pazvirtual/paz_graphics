#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

static DXGI_FORMAT dxgi_format(paz::TextureFormat format)
{
    if(format == paz::TextureFormat::RGBA16UNorm)
    {
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    if(format == paz::TextureFormat::Depth16UNorm)
    {
        return DXGI_FORMAT_D16_UNORM;
    }
    throw std::runtime_error("INCOMPLETE");
}

static D3D11_FILTER tex_filter(paz::MinMagFilter minFilter, paz::MinMagFilter
    magFilter, paz::MipmapFilter mipFilter)
{
    ??
}

static D3D11_ADDRESS_MODE address_mode(paz::WrapMode wrap)
{
    ??
}

static ID3D11SAMPLERState* create_sampler(paz::MinMagFilter minFilter, paz::
    MinMagFilter magFilter, paz::MipmapFilter mipFilter, paz::WrapMode wrapS,
    paz::WrapMode wrapT)
{
    D3D11_SAMPLER_DESC descriptor = {};
    descriptor->Filter = tex_filter(minFilter, magFilter, mipFilter);
    descriptor->AddressU = address_mode(wrapS);
    descriptor->AddressV = address_mode(wrapT);
    descriptor->AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    descriptor->MipLODBias = 0;
    descriptor->MaxAnisotropy = 1;
    descriptor->ComparisonFunc = D3D11_COMPARISON_NEVER; //TEMP
    descriptor->BorderColor = {};
    descriptor->MinLOD = 0;
    descriptor->MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* sampler;
    const auto hr = d3d_device()->CreateSamplerState(&descriptor, &sampler);
    if(hr)
    {
        throw std::runtime_error("Failed to create sampler (HRESULT " + std::
            to_string(hr) + ").");
    }
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
        _texture->Release();
    }
    if(_sampler)
    {
        _sampler->Release();
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
    _data->init(data);
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
    _data->init(image.bytes().data());
}

paz::Texture::Texture(RenderTarget&& target) : _data(std::move(target._data)) {}

void paz::Texture::Data::init(const void* data)
{
    // This is because of Metal restrictions.
    if((_format == TextureFormat::Depth16UNorm || _format == TextureFormat::
        Depth32Float) && _mipFilter != MipmapFilter::None)
    {
        throw std::runtime_error("Depth/stencil textures do not support mipmapp"
            "ing.");
    }
    D3D11_TEXTURE2D_DESC descriptor = {};
    descriptor.Width = _width;
    descriptor.Height = _height;
    descriptor.MipLevels = ??;
    descriptor.ArraySize = 1;
    descriptor.Format = dxgi_format(_format);
    descriptor.SampleDesc = {1, 0};
    descriptor.Usage = _isRenderTarget ? D3D11_USAGE_DEFAULT :
        D3D11_USAGE_DYNAMIC;
    descriptor.BindFlags = ??;
    descriptor.CPUAccessFlags = _isRenderTarget ? D3D11_CPU_ACCESS_READ : 0;
    descriptor.MiscFlags = _mipFilter != MipmapFilter::None ?
        D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
    HRESULT hr;
    if(data)
    {
        D3D11_SUBRESOURCE_DATA srData = {};
        srData.pSysMem = data;
        hr = d3d_device()->CreateTexture2D(&descriptor, &srData, &_texture);
    }
    else
    {
        hr = d3d_device()->CreateTexture2D(&descriptor, nullptr, &_texture);
    }
    if(hr)
    {
        throw std::runtime_error("Failed to create texture (HRESULT " + std::
            to_string(hr) + ").");
    }
    if(!_sampler)
    {
        _sampler = create_sampler(_minFilter, _magFilter, _mipFilter, _wrapS,
            _wrapT);
    }
}

void paz::Texture::Data::resize(int width, int height)
{
    if(_scale)
    {
        if(_texture)
        {
            _texture->Release();
        }
        _width = _scale*width;
        _height = _scale*height;
        init();
    }
}

void paz::Texture::Data::ensureMipmaps()
{
    throw std::logic_error("NOT IMPLEMENTED");
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
