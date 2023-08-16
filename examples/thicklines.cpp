#include "PAZ_Graphics"

static const std::string VertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
void main()
{
    gl_Position = vec4(pos.x, pos.y, 0, 1);
}
)===";

static const std::string GeomSrc = 1 + R"===(
layout(lines) in;
uniform int width;
uniform int height;
layout(triangle_strip, max_vertices = 4) out;
const float s = 2;
void main()
{
    vec2 para = (gl_in[1].gl_Position.xy - gl_in[0].gl_Position.xy);
    para /= length(para);
    vec2 perp = vec2(-para.y, para.x);
    para /= vec2(width, height);
    perp /= vec2(width, height);
    gl_Position = vec4(gl_in[0].gl_Position.xy - s*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy - s*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[0].gl_Position.xy + s*perp, 0, 1);
    EmitVertex();
    gl_Position = vec4(gl_in[1].gl_Position.xy + s*perp, 0, 1);
    EmitVertex();
    EndPrimitive();
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
    lib.geometry("geom", GeomSrc);
    lib.fragment("frag", FragSrc);

    const paz::Shader shader(lib, "vert", lib, "geom", lib, "frag");

    paz::RenderPass render(shader);

    paz::Window::Loop([&]()
    {
        render.begin({paz::LoadAction::Clear});
        render.uniform("width", paz::Window::ViewportWidth());
        render.uniform("height", paz::Window::ViewportHeight());
        render.primitives(paz::PrimitiveType::Lines, vertices);
        render.end();
    }
    );
}
