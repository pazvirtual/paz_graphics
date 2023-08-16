#include "PAZ_Graphics"

#include <unordered_set>

namespace paz
{
    void register_target(void* t);
    void unregister_target(void* t);
    void resize_targets();
    class Framebuffer;
    Framebuffer final_framebuffer();
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
