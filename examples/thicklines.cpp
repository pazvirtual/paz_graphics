#include "PAZ_Graphics"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
flat out float a;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
    gl_LineWidth = 4.;
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

static constexpr std::array<float, 4*5> points =
{
        0,     0,
     0.5f,     0,
        0,  0.5f,
    -0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

int main()
{
    paz::VertexBuffer vertices;
    vertices.attribute(2, points);

    paz::ShaderFunctionLibrary lib;
    lib.vertex("vert", VertSrc);
    lib.fragment("frag", FragSrc);

    const paz::Shader shader(lib, "vert", lib, "frag");

    paz::RenderPass render(shader);

    while(!paz::Window::Done())
    {
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }

        render.begin({paz::LoadAction::Clear});
        render.primitives(paz::PrimitiveType::LineStrip, vertices);
        render.end();

        paz::Window::EndFrame();
    }
}
