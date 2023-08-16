#include "PAZ_Graphics"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
flat out float a;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
    gl_LineWidth = 20.;
    a = float(gl_VertexID%2);
}
)===";

static const std::string FragSrc = 1 + R"===(
layout(location = 0) out vec4 color;
flat in float a;
void main()
{
    color = vec4(a, 1. - a, 1, 1);
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

int main(int argc, char** argv)
{
    int mode = 0;
    if(argc >= 2)
    {
        mode = std::stoi(argv[1]);
    }

    paz::VertexBuffer vertices;
    if(mode)
    {
        vertices.attribute(2, strip);
    }
    else
    {
        vertices.attribute(2, sep);
    }

    const paz::VertexFunction vert(VertSrc);
    const paz::FragmentFunction frag(FragSrc);

    paz::RenderPass render(vert, frag);

    while(!paz::Window::Done())
    {
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }

        render.begin({paz::LoadAction::Clear});
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
