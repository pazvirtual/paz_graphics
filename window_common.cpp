#include "window.hpp"

paz::Framebuffer paz::final_framebuffer()
{
    static const Framebuffer f = []()
    {
        Framebuffer f;
        f.attach(RenderTarget(TextureFormat::RGBA16UNorm));
        f.attach(RenderTarget(TextureFormat::Depth16UNorm));
        return f;
    }();
    return f;
}
