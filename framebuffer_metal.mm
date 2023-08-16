#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "common.hpp"
#include "internal_data.hpp"

paz::Framebuffer::Framebuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
    if(target._data->_format == TextureFormat::RGBA8UNorm_sRGB)
    {
        throw std::runtime_error("Drawing to sRGB textures is not supported.");
    }
    if(target._data->_format == TextureFormat::Depth16UNorm || target._data->
        _format == TextureFormat::Depth32Float)
    {
        if(_data->_depthStencilAttachment)
        {
            throw std::runtime_error("A depth/stencil target is already attache"
                "d");
        }
        _data->_depthStencilAttachment = target._data;
    }
    else
    {
        _data->_colorAttachments.push_back(target._data);
    }
    if(!target._data->_scale) //TEMP
    {
        _data->_width = target.width();
        _data->_height = target.height();
    }
}

paz::Texture paz::Framebuffer::colorAttachment(std::size_t i) const
{
    if(i >= _data->_colorAttachments.size())
    {
        throw std::runtime_error("Color attachment " + std::to_string(i) + " is"
            " out of bounds.");
    }
    Texture temp;
    temp._data = _data->_colorAttachments[i];
    return temp;
}

paz::Texture paz::Framebuffer::depthStencilAttachment() const
{
    if(!_data->_depthStencilAttachment)
    {
        throw std::runtime_error("Framebuffer has no depth/stencil attachment."
            );
    }
    Texture temp;
    temp._data = _data->_depthStencilAttachment;
    return temp;
}

#endif
