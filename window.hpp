#ifndef PAZ_GRAPHICS_WINDOW_HPP
#define PAZ_GRAPHICS_WINDOW_HPP

#include "PAZ_Graphics"
#include "detect_os.hpp"
#include <unordered_set>

namespace paz
{
    void register_target(void* t);
    void unregister_target(void* t);
    void resize_targets();
    Framebuffer final_framebuffer();
    unsigned char to_srgb(double x);
#ifndef PAZ_MACOS
    void begin_frame();
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
}

#endif
