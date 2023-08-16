#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "window.hpp"

paz::Framebuffer::Data::~Data()
{
}

paz::Framebuffer::Data::Data()
{
}

paz::Framebuffer::Framebuffer()
{
    initialize();

    _data = std::make_shared<Data>();
}

void paz::Framebuffer::attach(const RenderTarget& target)
{
}

paz::Texture paz::Framebuffer::colorAttachment(std::size_t i) const
{
}

paz::Texture paz::Framebuffer::depthStencilAttachment() const
{
}

#endif
