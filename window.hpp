#include "PAZ_Graphics"

#include <unordered_set>

namespace paz
{
    void register_target(void* t);
    void unregister_target(void* t);
    void resize_targets();
    struct Initializer
    {
        std::unordered_set<void*> _renderTargets;
        Initializer();
        ~Initializer();
    };
    // `paz::initialize` should be called at the beginning of every graphics
    // wrapper class constructor declared in PAZ_Graphics that is non-trivial on
    // all platforms and every method of `paz::Window`.
    Initializer& initialize();
}
