#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"

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
}

#endif
