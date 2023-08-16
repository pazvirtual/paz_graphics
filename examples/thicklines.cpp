#include "PAZ_Graphics"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 aPos;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)===";

static const std::string GeomSrc = 1 + R"===(
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
void main()
{
    gl_Position = gl_in[0].gl_Position + vec4(0,-0.1, 0, 0);
    EmitVertex();
    gl_Position = gl_in[1].gl_Position + vec4(0,-0.1, 0, 0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(0, 0.1, 0, 0);
    EmitVertex();
    gl_Position = gl_in[1].gl_Position + vec4(0, 0.1, 0, 0);
    EmitVertex();
    EndPrimitive();
}
)===";

static const std::string FragSrc = 1 + R"===(
layout(location = 0) out vec4 FragColor;
void main()
{
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)===";

static constexpr std::array<float, 8> points =
{
    -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, -0.5f, -0.5f
};

int main()
{
    paz::Window::SetCursorMode(paz::Window::CursorMode::Disable);

    paz::VertexBuffer vertices;
    vertices.attribute(2, points);

    paz::ShaderFunctionLibrary lib;
    lib.vertex("vert", VertSrc);
    lib.geometry("geom", GeomSrc);
    lib.fragment("frag", FragSrc);

    const paz::Shader shader(lib, "vert", lib, "geom", lib, "frag");

    paz::RenderPass render(shader);

    paz::Window::Loop([&]()
    {
        render.begin({paz::RenderPass::LoadAction::Clear});
        render.primitives(paz::RenderPass::PrimitiveType::Lines, vertices);
        render.end();
    }
    );
}
