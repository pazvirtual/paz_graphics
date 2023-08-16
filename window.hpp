#include "PAZ_Graphics"

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
    Initializer& initialize();
}
