#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

#define CASE(a, b) case paz::WrapMode::a: return D3D11_TEXTURE_ADDRESS_##b;

static DXGI_FORMAT dxgi_format(paz::TextureFormat format)
{
    if(format == paz::TextureFormat::RGBA16UNorm)
    {
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    }
    if(format == paz::TextureFormat::Depth16UNorm)
    {
        return DXGI_FORMAT_R16_TYPELESS;
    }
    throw std::runtime_error("INCOMPLETE");
}

static D3D11_FILTER tex_filter(paz::MinMagFilter minFilter, paz::MinMagFilter
    magFilter, paz::MipmapFilter mipFilter)
{
    if(mipFilter == paz::MipmapFilter::Nearest || mipFilter == paz::
        MipmapFilter::None)
    {
        if(minFilter == paz::MinMagFilter::Nearest)
        {
            if(magFilter == paz::MinMagFilter::Nearest)
            {
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
            else if(magFilter == paz::MinMagFilter::Linear)
            {
                return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            }
            else
            {
                throw std::runtime_error("Invalid magnification function.");
            }
        }
        else if(minFilter == paz::MinMagFilter::Linear)
        {
            if(magFilter == paz::MinMagFilter::Nearest)
            {
                return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            }
            else if(magFilter == paz::MinMagFilter::Linear)
            {
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            }
            else
            {
                throw std::runtime_error("Invalid magnification function.");
            }
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else if(mipFilter == paz::MipmapFilter::Linear)
    {
        if(minFilter == paz::MinMagFilter::Nearest)
        {
            if(magFilter == paz::MinMagFilter::Nearest)
            {
                return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            }
            else if(magFilter == paz::MinMagFilter::Linear)
            {
                return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            }
            else
            {
                throw std::runtime_error("Invalid magnification function.");
            }
        }
        else if(minFilter == paz::MinMagFilter::Linear)
        {
            if(magFilter == paz::MinMagFilter::Nearest)
            {
                return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
            else if(magFilter == paz::MinMagFilter::Linear)
            {
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            }
            else
            {
                throw std::runtime_error("Invalid magnification function.");
            }
        }
        else
        {
            throw std::runtime_error("Invalid minification function.");
        }
    }
    else
    {
        throw std::runtime_error("Invalid mipmap selection function.");
    }
}

static D3D11_TEXTURE_ADDRESS_MODE address_mode(paz::WrapMode m)
{
    switch(m)
    {
        CASE(Repeat, WRAP)
        CASE(MirrorRepeat, MIRROR)
        CASE(ClampToEdge, CLAMP)
        CASE(ClampToZero, BORDER)
    }

    throw std::logic_error("Invalid texture wrapping mode requested.");
}

static ID3D11SamplerState* create_sampler(paz::MinMagFilter minFilter, paz::
    MinMagFilter magFilter, paz::MipmapFilter mipFilter, paz::WrapMode wrapS,
    paz::WrapMode wrapT)
{
    D3D11_SAMPLER_DESC descriptor = {};
    descriptor.Filter = tex_filter(minFilter, magFilter, mipFilter);
    descriptor.AddressU = address_mode(wrapS);
    descriptor.AddressV = address_mode(wrapT);
    descriptor.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    descriptor.MaxAnisotropy = 1;
    descriptor.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* sampler;
    const auto hr = paz::d3d_device()->CreateSamplerState(&descriptor,
        &sampler);
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
    if(_resourceView)
    {
        _resourceView->Release();
    }
    if(_rtView)
    {
        _rtView->Release();
    }
    if(_dsView)
    {
        _dsView->Release();
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

    D3D11_TEXTURE2D_DESC descriptor = {};
    descriptor.Width = _width;
    descriptor.Height = _height;
    descriptor.MipLevels = _mipFilter == MipmapFilter::None;
    descriptor.ArraySize = 1;
    descriptor.Format = dxgi_format(_format);
    descriptor.SampleDesc.Count = 1;
    descriptor.Usage = _isRenderTarget || _mipFilter != MipmapFilter::None ?
        D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
    descriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if(_isRenderTarget && (_format == TextureFormat::Depth16UNorm || _format ==
        TextureFormat::Depth32Float))
    {
        descriptor.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if(_isRenderTarget || _mipFilter != MipmapFilter::None)
    {
        descriptor.BindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    descriptor.MiscFlags = _mipFilter == MipmapFilter::None ? 0 :
        D3D11_RESOURCE_MISC_GENERATE_MIPS;
    HRESULT hr;
    if(data)
    {
        throw std::logic_error("IMPLEMENT THIS");
        D3D11_SUBRESOURCE_DATA srData = {};
        // ...
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
    if(!_resourceView)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC rvDescriptor = {};
        if(_format == TextureFormat::Depth16UNorm)
        {
            rvDescriptor.Format = DXGI_FORMAT_R16_UNORM;
        }
        else if(_format == TextureFormat::Depth32Float)
        {
            rvDescriptor.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else
        {
            rvDescriptor.Format = dxgi_format(_format);
        }
        rvDescriptor.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        rvDescriptor.Texture2D.MipLevels = descriptor.MipLevels;
        const auto hr = d3d_device()->CreateShaderResourceView(_texture,
            &rvDescriptor, &_resourceView);
        if(hr)
        {
            throw std::runtime_error("Failed to create resource view (HRESULT "
                + std::to_string(hr) + ").");
        }
    }
    if(_isRenderTarget)
    {
        if(_format == TextureFormat::Depth16UNorm || _format == TextureFormat::
            Depth32Float)
        {
            if(!_dsView)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsDescriptor = {};
                dsDescriptor.Format = _format == TextureFormat::Depth16UNorm ?
                    DXGI_FORMAT_D16_UNORM : DXGI_FORMAT_D32_FLOAT;
                dsDescriptor.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                const auto hr = d3d_device()->CreateDepthStencilView(_texture,
                    &dsDescriptor, &_dsView);
                if(hr)
                {
                    throw std::runtime_error("Failed to create depth/stencil vi"
                        "ew (HRESULT " + std::to_string(hr) + ").");
                }
            }
        }
        else if(!_rtView)
        {
            const auto hr = d3d_device()->CreateRenderTargetView(_texture,
                nullptr, &_rtView);
            if(hr)
            {
                throw std::runtime_error("Failed to create render target view ("
                    "HRESULT " + std::to_string(hr) + ").");
            }
        }
    }

    ensureMipmaps();
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
    d3d_context()->GenerateMips(_resourceView);
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
