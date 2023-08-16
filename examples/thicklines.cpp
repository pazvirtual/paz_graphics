#include "PAZ_Graphics"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
    gl_LineWidth = 4.;
}
)===";

static const std::string FragSrc = 1 + R"===(
layout(location = 0) out vec4 color;
void main()
{
    color = vec4(0, 1, 0, 1);
}
)===";

static constexpr std::array<float, 4*5> points =
{
    0, 0, 0.5f, 0,
    0.5f, 0, 0, 0.5f,
    0, 0.5f, -0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, 0.5f, -0.5f
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
        render.begin({paz::LoadAction::Clear});
        render.uniform("width", paz::Window::ViewportWidth());
        render.uniform("height", paz::Window::ViewportHeight());
        render.primitives(paz::PrimitiveType::Lines, vertices);
        render.end();

        paz::Window::EndFrame();
    }
}
