#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#import "util_metal.hh"
#import "app_delegate.hh"
#import "view_controller.hh"
#import <MetalKit/MetalKit.h>

#define DEVICE [[(ViewController*)[[(AppDelegate*)[NSApp delegate] window] \
    contentViewController] mtkView] device]

static void clean(void* texture)
{
    if(texture)
    {
        [(id<MTLTexture>)texture release];
        texture = nullptr;
    }
}

static void init(void* texture, int width, int height, int numChannels, int
    numBits, paz::Texture::DataType type)
{
    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc]
        init];
    [textureDescriptor setPixelFormat:pixel_format(numChannels, numBits, type)];
    [textureDescriptor setWidth:width];
    [textureDescriptor setHeight:height];
    [textureDescriptor setUsage:MTLTextureUsageRenderTarget|
        MTLTextureUsageShaderRead];
    texture = [DEVICE newTextureWithDescriptor:textureDescriptor];
    [textureDescriptor release];
}

paz::RenderTarget::RenderTarget() {}

paz::RenderTarget::~RenderTarget()
{
    clean(_texture);
    paz::Window::UnregisterTarget(this);
}

paz::RenderTarget::RenderTarget(double scale, int numChannels, int numBits,
    DataType type, MinMagFilter minFilter, MinMagFilter magFilter)
{
    _scale = scale;
    _numChannels = numChannels;
    _numBits = numBits;
    _type = type;
    init(_texture, _scale*Window::ViewportWidth(), _scale*Window::
        ViewportHeight(), _numChannels, _numBits, _type);
    _sampler = create_sampler(minFilter, magFilter);
    paz::Window::RegisterTarget(this);
}

void paz::RenderTarget::resize(GLsizei width, GLsizei height)
{
    clean(_texture);
    init(_texture, _scale*width, _scale*height, _numChannels, _numBits, _type);
}

#endif
