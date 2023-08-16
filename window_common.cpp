#include "window.hpp"
#include <cmath>

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

paz::Image paz::flip_image(const paz::Image& image)
{
    if(image.height() < 2)
    {
        return image;
    }
    paz::Image flipped = image;
    const std::size_t bytesPerRow = image.bytes().size()/image.height();
    for(int i = 0; i < image.height(); ++i)
    {
        std::copy(image.bytes().begin() + bytesPerRow*i, image.bytes().begin() +
            bytesPerRow*(i + 1), flipped.bytes().begin() + bytesPerRow*(image.
            height() - i - 1));
    }
    return flipped;
}
