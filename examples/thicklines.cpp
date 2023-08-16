#include "PAZ_Graphics"
#include "PAZ_IO"

static constexpr double MinWidth = 1.;
static constexpr double MaxWidth = 9.;

static const std::string LineVertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
out float a;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0., 1.);
    a = float(gl_VertexID%2);
}
)===";

static const std::string LineFragSrc = 1 + R"===(
layout(location = 0) out vec4 color;
in float a;
void main()
{
    color = vec4(pow(0.5, 2.2)*vec3(a, 1. - a, 1.), 1.);
}
)===";

static constexpr std::array<float, 6*2> strip =
{
        0,     0,
     0.5f,     0,
        0,  0.5f,
    -0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

static constexpr std::array<float, 5*4> sep =
{
        0,     0,
     0.5f,     0,
     0.5f,     0,
        0,  0.5f,
        0,  0.5f,
    -0.5f,  0.5f,
    -0.5f,  0.5f,
    -0.5f, -0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

static int mode;
static paz::VertexBuffer vertices;
static void set_mode(int m)
{
    mode = m;
    vertices = paz::VertexBuffer();
    if(mode)
    {
        vertices.attribute(2, strip);
    }
    else
    {
        vertices.attribute(2, sep);
    }

}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    set_mode(0);

    paz::VertexBuffer quadVerts;
    quadVerts.attribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    const paz::VertexFunction lineVert0(LineVertSrc);
    const paz::VertexFunction lineVert1(paz::read_bytes(appDir + "/lines-x.vert"
        ).str());
    const paz::VertexFunction lineVert2(paz::read_bytes(appDir + "/lines-y.vert"
        ).str());
    const paz::VertexFunction quadVert(paz::read_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction lineFrag0(LineFragSrc);
    const paz::FragmentFunction lineFrag1(paz::read_bytes(appDir + "/lines.frag"
        ).str());
    const paz::FragmentFunction lineFrag2(paz::read_bytes(appDir +
        "/lines-post.frag").str());

    paz::Framebuffer buff0;
    buff0.attach(paz::RenderTarget(1., paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));

    paz::Framebuffer buff1;
    buff1.attach(paz::RenderTarget(1., paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));

    paz::Framebuffer buff2;
    buff2.attach(paz::RenderTarget(1., paz::TextureFormat::RGBA16Float));

    paz::RenderPass basePass(buff0, lineVert0, lineFrag0);
    paz::RenderPass xPass(buff1, lineVert1, lineFrag1);
    paz::RenderPass yPass(buff2, lineVert2, lineFrag1);
    paz::RenderPass postPass(quadVert, lineFrag2);

    double width = 0.5*(MaxWidth - MinWidth);
    while(!paz::Window::Done())
    {
        paz::Window::SetCursorMode(paz::Window::GamepadActive() ? paz::
            CursorMode::Disable : paz::CursorMode::Normal);

        if(paz::Window::KeyPressed(paz::Key::One) || paz::Window::
            GamepadPressed(paz::GamepadButton::X))
        {
            set_mode(0);
        }
        else if(paz::Window::KeyPressed(paz::Key::Two) || paz::Window::
            GamepadPressed(paz::GamepadButton::A))
        {
            set_mode(1);
        }
        else if(paz::Window::KeyPressed(paz::Key::Three) || paz::Window::
            GamepadPressed(paz::GamepadButton::B))
        {
            set_mode(2);
        }
        if(paz::Window::KeyDown(paz::Key::Up) || paz::Window::GamepadDown(paz::
            GamepadButton::Up))
        {
            width = std::min(width + 5.*paz::Window::FrameTime(), MaxWidth);
        }
        if(paz::Window::KeyDown(paz::Key::Down) || paz::Window::
            GamepadDown(paz::GamepadButton::Down))
        {
            width = std::max(width - 5.*paz::Window::FrameTime(), MinWidth);
        }
        if(paz::Window::KeyPressed(paz::Key::Q) || paz::Window::GamepadPressed(
            paz::GamepadButton::Back))
        {
            paz::Window::Quit();
        }

        basePass.begin({paz::LoadAction::Clear});
        if(mode == 0)
        {
            basePass.draw(paz::PrimitiveType::Lines, vertices);
        }
        else if(mode == 1)
        {
            basePass.draw(paz::PrimitiveType::LineStrip, vertices);
        }
        else
        {
            basePass.draw(paz::PrimitiveType::LineLoop, vertices);
        }
        basePass.end();

        xPass.begin();
        xPass.read("base", buff0.colorAttachment(0));
        xPass.uniform("texOffset", std::array<float, 2>{1.f/buff0.
            colorAttachment(0).width(), 1.f/buff0.colorAttachment(0).height()});
        xPass.uniform("width", static_cast<float>(width));
        xPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        xPass.end();

        yPass.begin();
        yPass.read("base", buff1.colorAttachment(0));
        yPass.uniform("texOffset", std::array<float, 2>{1.f/buff1.
            colorAttachment(0).width(), 1.f/buff1.colorAttachment(0).height()});
        yPass.uniform("width", static_cast<float>(width));
        yPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        yPass.end();

        postPass.begin();
        postPass.read("img", buff2.colorAttachment(0));
        postPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        postPass.end();

        paz::Window::EndFrame();
    }
}
