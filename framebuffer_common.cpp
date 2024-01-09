#include "PAZ_Graphics"
#include "common.hpp"
#include "internal_data.hpp"

int paz::Framebuffer::Data::width()
{
    if(!_colorAttachments.empty())
    {
        return _colorAttachments[0]->_width;
    }
    if(_depthStencilAttachment)
    {
        return _depthStencilAttachment->_width;
    }
    return 0;
}

int paz::Framebuffer::Data::height()
{
    if(!_colorAttachments.empty())
    {
        return _colorAttachments[0]->_height;
    }
    if(_depthStencilAttachment)
    {
        return _depthStencilAttachment->_height;
    }
    return 0;
}

paz::Framebuffer::Framebuffer()
{
    initialize();

    _data = std::make_shared<Data>();
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

int paz::Framebuffer::width() const
{
    return _data->width();
}

int paz::Framebuffer::height() const
{
    return _data->height();
}
