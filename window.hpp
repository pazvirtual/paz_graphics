#include "detect_os.hpp"
#include "PAZ_Graphics"

namespace paz
{
    void register_target(ColorTarget* target);
    void register_target(DepthStencilTarget* target);
    void unregister_target(ColorTarget* target);
    void unregister_target(DepthStencilTarget* target);
#ifdef PAZ_MACOS
    void draw_in_renderer();
#endif
    void resize_targets();
}
