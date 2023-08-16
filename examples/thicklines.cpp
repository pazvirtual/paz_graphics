#include "PAZ_Graphics"
#include "PAZ_IO"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
out float a;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
    gl_LineWidth = 20.;
    a = float(gl_VertexID%2);
}
)===";

static const std::string FragSrc = 1 + R"===(
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

int main()
{
    set_mode(0);

    const paz::VertexFunction vert(VertSrc);
    const paz::FragmentFunction frag(FragSrc);

    paz::RenderPass render(vert, frag, paz::BlendMode::Additive);

    while(!paz::Window::Done())
    {
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
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }

        render.begin({paz::LoadAction::Clear});
        render.cull(paz::CullMode::Back);
        if(mode == 0)
        {
            render.primitives(paz::PrimitiveType::Lines, vertices);
        }
        else if(mode == 1)
        {
            render.primitives(paz::PrimitiveType::LineStrip, vertices);
        }
        else
        {
            render.primitives(paz::PrimitiveType::LineLoop, vertices);
        }
        render.end();

        paz::Window::EndFrame();
    }
}
