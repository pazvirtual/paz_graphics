#include "detect_os.hpp"
#include "PAZ_Graphics"

namespace paz
{
#ifdef PAZ_MACOS
    void draw_in_renderer();
#endif
    void register_target(void* data);
    void unregister_target(void* data);
    void resize_targets();
    void initialize();
}
