#include "PAZ_Graphics"
#include "PAZ_IO"

static constexpr float ZNear = 1.;
static constexpr float ZFar = 10.;
static constexpr float YFov = 1.;

static const std::string ZQuadVertSrc = 1 + R"===(
uniform mat4 proj;
uniform float z;
layout(location = 0) in vec2 pos;
out float _z;
void main()
{
    gl_Position = mul(proj, vec4(-0.5*(z + 4.) + cos(0.5)*pos.x, pos.y, z + sin(
        0.5)*pos.x, 1.));
    _z = z;
}
)===";

static const std::string OitFragSrc = 1 + R"===(
uniform vec4 col;
in float _z;
layout(location = 0) out vec4 accum;
layout(location = 1) out vec4 r;
void main()
{
    vec3 ci = col.a*col.rgb;
    float ai = col.a;
    float w = ai*pow(abs(_z), -6);
    accum = vec4(ci, ai)*w;
    r = vec4(0., 0., 0., ai);
}
)===";

static const std::string FinalFragSrc = 1 + R"===(
uniform sampler2D accumTex;
uniform sampler2D revealageTex;
in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 accum = texture(accumTex, uv);
    float r = texture(revealageTex, uv).a;
    color = vec4(accum.rgb/max(accum.a, 1e-5), r);
}
)===";

static const std::string ZQuadFragSrc = 1 + R"===(
uniform vec4 col;
in float _z;
layout(location = 0) out vec4 color;
void main()
{
    color = col;
}
)===";

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    paz::Window::SetTitle("PAZ_Graphics: transparency");

    paz::VertexBuffer quadVertices;
    quadVertices.addAttribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1,
        1});

    const paz::VertexFunction vert(ZQuadVertSrc);
    const paz::FragmentFunction frag(ZQuadFragSrc);
    paz::RenderPass pass(vert, frag, {paz::BlendMode::SrcAlpha_InvSrcAlpha});

    paz::RenderTarget accumTex(paz::TextureFormat::RGBA16Float);
    paz::RenderTarget revealageTex(paz::TextureFormat::RGBA8UNorm);

    paz::Framebuffer buff;
    buff.attach(accumTex);
    buff.attach(revealageTex);

    const paz::VertexFunction oitVert(ZQuadVertSrc);
    const paz::FragmentFunction oitFrag(OitFragSrc);
    const paz::VertexFunction finalVert(paz::read_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction finalFrag(FinalFragSrc);
    paz::RenderPass pass0(buff, oitVert, oitFrag, {paz::BlendMode::One_One,
        paz::BlendMode::Zero_InvSrcAlpha});
    paz::RenderPass pass1(finalVert, finalFrag, {paz::BlendMode::
        InvSrcAlpha_SrcAlpha});

    while(!paz::Window::Done())
    {
        paz::Window::PollEvents();
        paz::Window::SetCursorMode(paz::Window::GamepadActive() ? paz::
            CursorMode::Disable : paz::CursorMode::Normal);
        if(paz::Window::KeyPressed(paz::Key::Q) || paz::Window::GamepadPressed(
            paz::GamepadButton::Back))
        {
            paz::Window::Quit();
        }
        if(paz::Window::KeyPressed(paz::Key::F) || paz::Window::GamepadPressed(
            paz::GamepadButton::Start))
        {
            paz::Window::IsFullscreen() ? paz::Window::MakeWindowed() : paz::
                Window::MakeFullscreen();
        }

        const auto proj = paz::perspective(YFov, paz::Window::AspectRatio(),
            ZNear, ZFar);

        // Draw opaque geometry.
        pass.begin({paz::LoadAction::Clear});
        pass.uniform("proj", proj);
        pass.uniform("z", -6.f);
        pass.uniform("col", 0.5f, 0.f, 0.5f, 0.9f);
        pass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
        pass.end();

        // Draw transparent geometry.
        if(paz::Window::KeyDown(paz::Key::Space) || paz::Window::GamepadDown(
            paz::GamepadButton::A)) // OIT
        {
            pass0.begin({paz::LoadAction::FillZeros, paz::LoadAction::
                FillOnes});
            pass0.uniform("proj", proj);
            pass0.uniform("z", -4.f);
            pass0.uniform("col", 1.f, 0.f, 0.f, 0.9f);
            pass0.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            pass0.uniform("z", -5.f);
            pass0.uniform("col", 1.f, 1.f, 0.f, 0.5f);
            pass0.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            pass0.end();

            pass1.begin();
            pass1.read("accumTex", accumTex);
            pass1.read("revealageTex", revealageTex);
            pass1.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            pass1.end();
        }
        else // Ordered
        {
            pass.begin();
            pass.uniform("proj", proj);
            pass.uniform("z", -5.f);
            pass.uniform("col", 1.f, 1.f, 0.f, 0.5f);
            pass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            pass.uniform("z", -4.f);
            pass.uniform("col", 1.f, 0.f, 0.f, 0.9f);
            pass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            pass.end();
        }

        paz::Window::EndFrame();
    }
}
