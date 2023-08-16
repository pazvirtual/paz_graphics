#include "PAZ_Graphics"

namespace paz
{
    void register_target(void* data);
    void unregister_target(void* data);
    void resize_targets();
    struct Initializer
    {
        std::unordered_set<void*> renderTargets;
        Initializer();
        ~Initializer();
    };
    Initializer& initialize();
}
