#include "PAZ_Graphics"
#include "PAZ_IO"
#include <numeric>

static constexpr double MinWidth = 1.;
static constexpr double MaxWidth = 9.;

static const std::string LineVertSrc = 1 + R"===(
layout(location = 0) in vec2 pos;
layout(location = 1) in int sampleIdx [[instance]];
uniform int width;
uniform int height;
out float a;
void main()
{
    // Note: Multiply offsets by `gl_Position.w` if 3D (perspective divide).
    float xOffset = (2*(sampleIdx/4) - 3)/(3.*width);
    float yOffset = (2*(sampleIdx%4) - 3)/(3.*height);
    gl_Position = vec4(pos.x + xOffset, pos.y + yOffset, 0., 1.);
    a = float(gl_VertexID%2);
}
)===";

static const std::string LineFragSrc = 1 + R"===(
layout(location = 0) out vec4 color;
uniform uint numSamples;
in float a;
void main()
{
    color = 0.2176*vec4(vec3(a, 1. - a, 1.), 1.)/numSamples;
}
)===";

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

static constexpr std::array<float, 6*2> strip =
{
        0,     0,
     0.5f,     0,
        0,  0.5f,
    -0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

static constexpr std::array<float, 7*2> loop =
{
        0,     0,
     0.5f,     0,
        0,  0.5f,
    -0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f,
        0,     0
};

static int mode;
static paz::VertexBuffer vertices;
static void set_mode(int m)
{
    mode = m;
    vertices = paz::VertexBuffer();
    switch(mode)
    {
        case 0: vertices.addAttribute(2, sep); break;
        case 1: vertices.addAttribute(2, strip); break;
        default: vertices.addAttribute(2, loop); break;
    }

}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    set_mode(0);

    paz::VertexBuffer quadVerts;
    quadVerts.addAttribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    const paz::VertexFunction lineVert0(LineVertSrc);
    const paz::VertexFunction lineVert1(paz::read_bytes(appDir + "/lines-x.vert"
        ).str());
    const paz::VertexFunction lineVert2(paz::read_bytes(appDir + "/lines-y.vert"
        ).str());
    const paz::FragmentFunction lineFrag0(LineFragSrc);
    const paz::FragmentFunction lineFrag1(paz::read_bytes(appDir + "/lines.frag"
        ).str());

    paz::Framebuffer buff0;
    buff0.attach(paz::RenderTarget(paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));

    paz::Framebuffer buff1;
    buff1.attach(paz::RenderTarget(paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));

    paz::RenderPass basePass(buff0, lineVert0, lineFrag0, {paz::BlendMode::
        One_One});
    paz::RenderPass xPass(buff1, lineVert1, lineFrag1);
    paz::RenderPass yPass(lineVert2, lineFrag1);

    paz::InstanceBuffer samples;
    {
        std::array<int, 16> indices;
        std::iota(indices.begin(), indices.end(), 0);
        samples.addAttribute(1, indices);
    }

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
        if(paz::Window::KeyPressed(paz::Key::F) || paz::Window::GamepadPressed(
            paz::GamepadButton::Start))
        {
            paz::Window::IsFullscreen() ? paz::Window::MakeWindowed() : paz::
                Window::MakeFullscreen();
        }

        basePass.begin({paz::LoadAction::Clear});
        basePass.uniform("width", paz::Window::ViewportWidth());
        basePass.uniform("height", paz::Window::ViewportHeight());
        basePass.uniform("numSamples", static_cast<unsigned int>(samples.
            size()));
        if(mode == 0)
        {
            basePass.draw(paz::PrimitiveType::Lines, vertices, samples);
        }
        else if(mode == 1)
        {
            basePass.draw(paz::PrimitiveType::LineStrip, vertices, samples);
        }
        else
        {
            basePass.draw(paz::PrimitiveType::LineStrip, vertices, samples);
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

        paz::Window::EndFrame();
    }
}
