#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "render_pass.hpp"
#include "internal_data.hpp"
#include "util_d3d11.hpp"
#include "window.hpp"

paz::RenderPass::RenderPass()
{
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const VertexFunction& vert,
    const FragmentFunction& frag, BlendMode mode)
{
}

paz::RenderPass::RenderPass(const VertexFunction& vert, const FragmentFunction&
    frag, BlendMode mode) : RenderPass(final_framebuffer(), vert, frag, mode) {}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
}

void paz::RenderPass::depth(DepthTestMode mode)
{
}

void paz::RenderPass::end()
{
}

void paz::RenderPass::cull(CullMode mode)
{
}

void paz::RenderPass::read(const std::string& name, const Texture& tex)
{
}

void paz::RenderPass::uniform(const std::string& name, int x)
{
}

void paz::RenderPass::uniform(const std::string& name, int x, int y)
{
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
{
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w)
{
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size)
{
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x)
{
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y)
{
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z)
{
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w)
{
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size)
{
}

void paz::RenderPass::uniform(const std::string& name, float x)
{
}

void paz::RenderPass::uniform(const std::string& name, float x, float y)
{
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z)
{
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w)
{
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size)
{
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices)
{
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices)
{
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances)
{
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances, const IndexBuffer& indices)
{
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
}

void paz::disable_blend_depth_cull()
{
}

#endif
