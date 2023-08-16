#include "detect_os.hpp"

#ifdef PAZ_MACOS

#include "PAZ_Graphics"
#include "window.hpp"
#include "internal_data.hpp"
#import <MetalKit/MetalKit.h>

paz::Shader::Shader(const paz::VertexFunction& vert, const paz::
    FragmentFunction& frag)
{
    initialize();

    _data = std::make_shared<Data>();

    _data->_vert = vert;
    _data->_frag = frag;
}

#endif
