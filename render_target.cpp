#include "PAZ_Graphics"
#include "util.hpp"
#include "internal_data.hpp"

paz::RenderTarget::RenderTarget()
{
    _data = std::make_unique<Data>();
}
