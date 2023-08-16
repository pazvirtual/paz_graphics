#include "PAZ_Graphics"
#include "PAZ_IO"

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
    const paz::VertexFunction quadVert(paz::load_file(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction lineFrag(LineFragSrc);
    const paz::FragmentFunction sdfFrag(paz::load_file(appDir + "/sdf.frag").
        str());

    paz::Framebuffer buff;
    buff.attach(paz::RenderTarget(1., paz::TextureFormat::RGBA16Float));

    paz::RenderPass basePass(buff, lineVert, lineFrag);
    paz::RenderPass sdfPass(quadVert, sdfFrag);

    int width = 1;
    while(!paz::Window::Done())
    {
        const double displayScale = static_cast<double>(paz::Window::
            ViewportWidth())/paz::Window::Width();

        if(paz::Window::KeyPressed(paz::Key::One))
        {
            set_mode(0);
        }
        else if(paz::Window::KeyPressed(paz::Key::Two))
        {
            set_mode(1);
        }
        else if(paz::Window::KeyPressed(paz::Key::Three))
        {
            set_mode(2);
        }
        if(paz::Window::KeyPressed(paz::Key::Up))
        {
            width = std::min(width + 1, 10);
        }
        if(paz::Window::KeyPressed(paz::Key::Down))
        {
            width = std::max(width - 1, 1);
        }
        if(paz::Window::KeyPressed(paz::Key::Q))
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
        sdfPass.uniform("width", static_cast<float>(displayScale*width));
        sdfPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        sdfPass.end();

        paz::Window::EndFrame();
    }
}
