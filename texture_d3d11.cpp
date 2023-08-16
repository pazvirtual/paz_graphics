#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "util_d3d11.hpp"
#include "internal_data.hpp"
#include "window.hpp"

#define CASE0(a, b) case paz::TextureFormat::a: return DXGI_FORMAT_##b;
#define CASE1(a, b) case paz::WrapMode::a: return D3D11_TEXTURE_ADDRESS_##b;

static DXGI_FORMAT dxgi_format(paz::TextureFormat format)
{
    switch(format)
    {
        CASE0(R8UInt, R8_UINT)
        CASE0(R8SInt, R8_SINT)
        CASE0(R8UNorm, R8_UNORM)
        CASE0(R8SNorm, R8_SNORM)
        CASE0(R16UInt, R16_UINT)
        CASE0(R16SInt, R16_SINT)
        CASE0(R16UNorm, R16_UNORM)
        CASE0(R16SNorm, R16_SNORM)
        CASE0(R16Float, R16_FLOAT)
        CASE0(R32UInt, R32_UINT)
        CASE0(R32SInt, R32_SINT)
        CASE0(R32Float, R32_FLOAT)

        CASE0(RG8UInt, R8G8_UINT)
        CASE0(RG8SInt, R8G8_SINT)
        CASE0(RG8UNorm, R8G8_UNORM)
        CASE0(RG8SNorm, R8G8_SNORM)
        CASE0(RG16UInt, R16G16_UINT)
        CASE0(RG16SInt, R16G16_SINT)
        CASE0(RG16UNorm, R16G16_UNORM)
        CASE0(RG16SNorm, R16G16_SNORM)
        CASE0(RG16Float, R16G16_FLOAT)
        CASE0(RG32UInt, R32G32_UINT)
        CASE0(RG32SInt, R32G32_SINT)
        CASE0(RG32Float, R32G32_FLOAT)

        CASE0(RGBA8UInt, R8G8B8A8_UINT)
        CASE0(RGBA8SInt, R8G8B8A8_SINT)
        CASE0(RGBA8UNorm, R8G8B8A8_UNORM)
        CASE0(RGBA8UNorm_sRGB, R8G8B8A8_UNORM_SRGB)
        CASE0(RGBA8SNorm, R8G8B8A8_SNORM)
        CASE0(RGBA16UInt, R16G16B16A16_UINT)
        CASE0(RGBA16SInt, R16G16B16A16_SINT)
        CASE0(RGBA16UNorm, R16G16B16A16_UNORM)
        CASE0(RGBA16SNorm, R16G16B16A16_SNORM)
        CASE0(RGBA16Float, R16G16B16A16_FLOAT)
        CASE0(RGBA32UInt, R32G32B32A32_UINT)
        CASE0(RGBA32SInt, R32G32B32A32_SINT)
        CASE0(RGBA32Float, R32G32B32A32_FLOAT)

        CASE0(Depth16UNorm, R16_TYPELESS)
        CASE0(Depth32Float, R32_TYPELESS)

        CASE0(BGRA8UNorm, B8G8R8A8_UNORM)
    }

    throw std::runtime_error("Invalid texture format requested.");
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
        CASE1(Repeat, WRAP)
        CASE1(MirrorRepeat, MIRROR)
        CASE1(ClampToEdge, CLAMP)
        CASE1(ClampToZero, BORDER)
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
            _texture = nullptr;
        }
        if(_resourceView)
        {
            _resourceView->Release();
            _resourceView = nullptr;
        }
        if(_rtView)
        {
            _rtView->Release();
            _rtView = nullptr;
        }
        if(_dsView)
        {
            _dsView->Release();
            _dsView = nullptr;
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
