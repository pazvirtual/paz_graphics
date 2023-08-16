#include "detect_os.hpp"

#ifdef PAZ_WINDOWS

#include "PAZ_Graphics"
#include "internal_data.hpp"
#include "util_d3d11.hpp"
#include "window.hpp"

#define CHECK_PASS if(!CurPass) throw std::logic_error("No current render pass"\
    "."); else if(this != CurPass) throw std::logic_error("Render pass operati"\
    "ons cannot be interleaved.");

static const paz::RenderPass* CurPass;

paz::RenderPass::RenderPass()
{
    initialize();
}

paz::RenderPass::RenderPass(const Framebuffer& fbo, const VertexFunction& vert,
    const FragmentFunction& frag, BlendMode mode)
{
    initialize();

    _data = std::make_shared<Data>();
}

paz::RenderPass::RenderPass(const VertexFunction& vert, const FragmentFunction&
    frag, BlendMode mode) : RenderPass(final_framebuffer(), vert, frag, mode) {}

void paz::RenderPass::begin(const std::vector<LoadAction>& colorLoadActions,
    LoadAction depthLoadAction)
{
    if(CurPass)
    {
        throw std::logic_error("Previous render pass was not ended.");
    }
    CurPass = this;

    if(!_data)
    {
        throw std::runtime_error("Render pass has not been initialized.");
    }

    begin_frame();
}

void paz::RenderPass::depth(DepthTestMode mode)
{
    CHECK_PASS
}

void paz::RenderPass::end()
{
    CHECK_PASS
    // ...
    CurPass = nullptr;
}

void paz::RenderPass::cull(CullMode mode)
{
    CHECK_PASS
}

void paz::RenderPass::read(const std::string& name, const Texture& tex)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, int x, int y, int z, int
    w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const int* x, std::size_t
    size)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, unsigned int x, unsigned
    int y, unsigned int z, unsigned int w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const unsigned int* x,
    std::size_t size)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, float x, float y, float
    z, float w)
{
    CHECK_PASS
}

void paz::RenderPass::uniform(const std::string& name, const float* x, std::
    size_t size)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const IndexBuffer& indices)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances)
{
    CHECK_PASS
}

void paz::RenderPass::draw(PrimitiveType type, const VertexBuffer& vertices,
    const InstanceBuffer& instances, const IndexBuffer& indices)
{
    CHECK_PASS
}

paz::Framebuffer paz::RenderPass::framebuffer() const
{
}

#endif
