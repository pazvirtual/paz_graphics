#include "detect_os.hpp"
#include "PAZ_Graphics"

namespace paz
{
    void register_target(RenderTarget* target);
    void unregister_target(RenderTarget* target);
#ifdef PAZ_MACOS
    void draw_in_renderer();
#endif
    void resize_targets();
}
