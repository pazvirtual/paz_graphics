#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[static_cast<ViewController*>([[static_cast<AppDelegate*>( \
    [NSApp delegate]) window] contentViewController]) mtkView] device]

paz::Framebuffer::~Framebuffer()
{
    _data->_colorAttachments.clear();
}

paz::Framebuffer::Framebuffer()
{
    _data = std::make_unique<Data>();
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
    if(target._data->_format == Texture::Format::Depth16UNorm || target._data->
        _format == Texture::Format::Depth32Float)
    {
        if(_data->_depthAttachment)
        {
            throw std::runtime_error("A depth/stencil target is already attache"
                "d");
        }
        _data->_depthAttachment = &target;
    }
    else
    {
        _data->_colorAttachments.push_back(&target);
    }
}

#endif
