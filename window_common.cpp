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

unsigned char paz::to_srgb(double x)
{
    // Convert to sRGB.
    x = x < 0.0031308 ? 12.92*x : 1.055*std::pow(x, 1./2.4) - 0.055;

    // Convert to normalized.
    return std::round(x*255.);
}
