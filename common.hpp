#ifndef PAZ_GRAPHICS_COMMON_HPP
#define PAZ_GRAPHICS_COMMON_HPP

#include "PAZ_Graphics"
#include "detect_os.hpp"
#ifdef PAZ_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#endif
#include <unordered_set>
#include <cstdint>

namespace paz
{
    void register_target(void* t);
    void unregister_target(void* t);
    void resize_targets();
    Framebuffer final_framebuffer();
    unsigned char to_srgb(double x);
    Image flip_image(const Image& img);
#ifndef PAZ_MACOS
    void begin_frame();
#endif
#ifdef PAZ_WINDOWS
    ID3D11Device* d3d_device();
    ID3D11DeviceContext* d3d_context();
#endif
    struct Initializer
    {
        std::unordered_set<void*> _renderTargets;
        Initializer();
        ~Initializer();
    };
    // `paz::initialize` should be called at the beginning of 1. every
    // constructor of every graphics wrapper class declared in PAZ_Graphics that
    // does not inherit from another wrapper class that does so and 2. every
    // method of `paz::Window`.
    Initializer& initialize();
    float half_to_float(std::uint16_t x);
    std::uint16_t float_to_half(float x);
}

#endif
