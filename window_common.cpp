#include "window.hpp"

paz::Framebuffer paz::final_framebuffer()
{
    static const Framebuffer f = []()
    {
        Framebuffer f;
        f.attach(RenderTarget(1., TextureFormat::RGBA8UNorm));
        f.attach(RenderTarget(1., TextureFormat::Depth16UNorm));
        return f;
    }();
    return f;
}
