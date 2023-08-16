#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "app_delegate.hh"
#import "view_controller.hh"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::Framebuffer::~Framebuffer()
{
    _data->_colorAttachments.clear();
}

void paz::Framebuffer::attach(const ColorTarget& target)
{
    _data->_colorAttachments.push_back(&target);
}

void paz::Framebuffer::attach(const DepthStencilTarget& target)
{
    if(_data->_depthAttachment)
    {
        throw std::runtime_error("A depth/stencil target is already attached");
    }
    _data->_depthAttachment = &target;
}

paz::Framebuffer::Framebuffer()
{
    _data = std::make_unique<Data>();
}

#endif
