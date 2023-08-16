#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "app_delegate.h"
#import "view_controller.h"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

void paz::Framebuffer::clean()
{
    colorAttachments.clear();
    depthAttachment = nullptr;
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
    colorAttachments.push_back(&target);
}

void paz::Framebuffer::attach(const DepthStencilTarget& target)
{
    if(depthAttachment)
    {
        throw std::runtime_error("A depth/stencil target is already attached");
    }
    depthAttachment = &target;
}

paz::Framebuffer::Framebuffer() {}

#endif
