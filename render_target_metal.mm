#include "PAZ_Graphics"

#ifdef PAZ_MACOS

#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

paz::RenderTarget::RenderTarget() {}

void paz::RenderTarget::clean()
{
    if(_texture)
    {
        [(id<MTLTexture>)_texture release];
        _texture = nullptr;
    }
}

void paz::RenderTarget::init(int width, int height)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:pixel_format(_numChannels, _numBits,
        _type)];
    [textureDescriptor setWidth:width];
    [textureDescriptor setHeight:height];
    [textureDescriptor setUsage:MTLTextureUsageRenderTarget|
        MTLTextureUsageShaderRead];
    _texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
}

paz::RenderTarget::~RenderTarget()
{
    clean();
    paz::Window::UnregisterTarget(this);
}

paz::RenderTarget::RenderTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    _numChannels = numChannels;
    _numBits = numBits;
    _type = type;
    init(_scale*Window::Width(), _scale*Window::Height());
    createSampler(minFilter, magFilter);
    paz::Window::RegisterTarget(this);
}

void paz::RenderTarget::resize(GLsizei width, GLsizei height)
{
    clean();
    init(_scale*width, _scale*height);
}

#endif
