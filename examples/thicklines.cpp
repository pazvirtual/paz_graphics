#include "PAZ_Graphics"
#include "PAZ_IO"

static constexpr int MinWidth = 2;
static constexpr int MaxWidth = 10;

static const std::string LineVertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
out float a;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
    a = float(gl_VertexID%2);
}
)===";

static const std::string LineFragSrc = 1 + R"===(
layout(location = 0) out vec4 color;
in float a;
void main()
{
    color = 0.5*vec4(a, 1. - a, 1, 1.);
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

    const paz::VertexFunction lineVert(LineVertSrc);
    const paz::VertexFunction quadVert(paz::load_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction lineFrag0(LineFragSrc);
    const paz::FragmentFunction lineFrag1(paz::load_bytes(appDir + "/lines.frag"
        ).str());

    paz::Framebuffer buff;
    buff.attach(paz::RenderTarget(1., paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));

    paz::RenderPass basePass(buff, lineVert, lineFrag0);
    paz::RenderPass sdfPass(quadVert, lineFrag1);

    int width = MinWidth;
    while(!paz::Window::Done())
    {
        paz::Window::SetCursorMode(paz::Window::GamepadActive() ? paz::
            CursorMode::Disable : paz::CursorMode::Normal);

        const double displayScale = static_cast<double>(paz::Window::
            ViewportWidth())/paz::Window::Width();

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
        if(paz::Window::KeyPressed(paz::Key::Up) || paz::Window::GamepadPressed(
            paz::GamepadButton::Up))
        {
            width = std::min(width + 1, MaxWidth);
        }
        if(paz::Window::KeyPressed(paz::Key::Down) || paz::Window::
            GamepadPressed(paz::GamepadButton::Down))
        {
            width = std::max(width - 1, MinWidth);
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

        sdfPass.begin({paz::LoadAction::Clear});
        sdfPass.read("base", buff.colorAttachment(0));
        sdfPass.uniform("width", static_cast<int>(displayScale*width));
        sdfPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        sdfPass.end();

        paz::Window::EndFrame();
    }
}
