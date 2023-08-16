#include "PAZ_Graphics"
#include <sstream>
#include <fstream>
#include <cmath>

static constexpr double BaseLength = 0.25;
static constexpr std::array<float, 6> TriPosData0 =
{
             0,  0.05,
    BaseLength,     0,
             0, -0.05
};
static constexpr std::array<float, 6> TriPosData1 =
{
             0,  0.1,
    BaseLength,    0,
             0, -0.1
};
static constexpr std::array<float, 12> TriColorData0 =
{
    1,   0, 1, 1,
    0,   1, 1, 1,
    1, 0.5, 0, 1
};
static constexpr std::array<float, 12> TriColorData1 =
{
    1, 0, 0, 1,
    1, 0, 0, 1,
    1, 0, 0, 1
};

static constexpr std::array<float, 8> QuadPosData =
{
     1, -1,
     1,  1,
    -1, -1,
    -1,  1
};

#ifndef M_PI
#define M_PI 3.14159265358979323846264338328
#endif

static double fract(const double n)
{
    return n - std::floor(n);
}

static double normalize_angle(const double n)
{
    return fract(n/(2.*M_PI))*2.*M_PI;
}

static void init(paz::RenderPass& scenePass, paz::RenderPass& textPass, paz::
    RenderPass& postPass, const std::string& appDir)
{
    paz::RenderTarget render(1., paz::TextureFormat::RGBA16Float);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    paz::ShaderFunctionLibrary shaders;
    shaders.vertex("shader", paz::load_file(appDir + "/shader.vert").str());
    shaders.vertex("font", paz::load_file(appDir + "/font.vert").str());
    shaders.vertex("quad", paz::load_file(appDir + "/quad.vert").str()),
    shaders.fragment("shader", paz::load_file(appDir + "/shader.frag").str());
    shaders.fragment("font", paz::load_file(appDir + "/font.frag").str());
    shaders.fragment("post", paz::load_file(appDir + "/post.frag").str());

    const paz::Shader sceneShader(shaders, "shader", shaders, "shader");
    const paz::Shader textShader(shaders, "font", shaders, "font");
    const paz::Shader postShader(shaders, "quad", shaders, "post");

    scenePass = paz::RenderPass(renderFramebuffer, sceneShader);
    textPass = paz::RenderPass(renderFramebuffer, textShader, paz::BlendMode::
        Additive);
    postPass = paz::RenderPass(postShader);
}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    const std::string msg = paz::load_file(appDir + "/msg.txt").str();

    paz::Window::SetMinSize(640, 480);

    const paz::Texture font(paz::load_pbm(appDir + "/font.pbm"));

    paz::RenderPass scenePass, textPass, postPass;
    init(scenePass, textPass, postPass, appDir);

    paz::VertexBuffer triVertices0;
    triVertices0.attribute(2, TriPosData0);
    triVertices0.attribute(4, TriColorData0);

    paz::VertexBuffer triVertices1;
    triVertices1.attribute(2, TriPosData1);
    triVertices1.attribute(4, TriColorData1);

    const paz::IndexBuffer loopIndices({0, 1, 2, 0});

    paz::VertexBuffer quadVertices;
    quadVertices.attribute(2, QuadPosData);

    double x = 0.;
    double y = 0.;

    double angle = 0.;
    double length = 1.;

    unsigned int mode = 0;

    paz::Window::Loop([&]()
    {
        // Handle events.
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }
        if(paz::Window::KeyPressed(paz::Key::F))
        {
            paz::Window::IsFullscreen() ? paz::Window::MakeWindowed() : paz::
                Window::MakeFullscreen();
        }
        if(paz::Window::MousePressed(0))
        {
            mode = !mode;
        }

        // Propagate physics.
        std::pair<double, double> m = paz::Window::MousePos();
        m.first = std::min(m.first, static_cast<double>(paz::Window::Width()));
        m.second = std::min(m.second, static_cast<double>(paz::Window::
            Height()));
        m.first = std::max(m.first*2./paz::Window::Height(), 0.);
        m.second = std::max(m.second*2./paz::Window::Height(), 0.);
        m.first -= paz::Window::AspectRatio();
        m.second -= 1.;
        const double deltaX = m.first - x;
        const double deltaY = m.second - y;
        const double dist = std::sqrt(deltaX*deltaX + deltaY*deltaY);
        length = std::max(1., dist/BaseLength);

        // Drawing prep.
        if(dist > 1e-2)
        {
            const double t = std::atan2(deltaY, deltaX);
            const double delta = normalize_angle(t - angle + M_PI) - M_PI;
            angle = normalize_angle(angle + 0.5*delta);

            const double k = std::min(10.*paz::Window::FrameTime(), 1.);
            x += k*deltaX;
            y += k*deltaY;
        }

        // Drawing.
        scenePass.begin({paz::LoadAction::Clear});
        scenePass.uniform("angle", static_cast<float>(angle));
        scenePass.uniform("aspectRatio", static_cast<float>(paz::Window::
            AspectRatio()));
        scenePass.uniform("origin", static_cast<float>(x), static_cast<float>(
            y));
        scenePass.uniform("length", static_cast<float>(length));
        if(mode)
        {
            scenePass.indexed(paz::PrimitiveType::LineStrip, triVertices1,
                loopIndices);
        }
        else
        {
            scenePass.primitives(paz::PrimitiveType::Triangles, triVertices0);
        }
        scenePass.end();

        textPass.begin({paz::LoadAction::Load}, paz::LoadAction::DontCare);
        textPass.read("font", font);
        textPass.uniform("baseSize", font.height());
        textPass.uniform("aspectRatio", paz::Window::AspectRatio());
        int row = 0;
        int col = 0;
        std::stringstream fullMsg;
        fullMsg << msg << std::round(1./paz::Window::FrameTime()) << " fps";
        for(const auto& n : fullMsg.str())
        {
            int c = -1;
            if(n >= '0' && n <= '9')
            {
                c = n - '0';
            }
            else if(n >= 'a' && n <= 'z')
            {
                c = n - 'a' + 10;
            }
            else if(n >= 'A' && n <= 'Z')
            {
                c = n - 'A' + 10;
            }
            else if(n == ':')
            {
                c = 36;
            }
            else if(n == '\n')
            {
                ++row;
                col = 0;
                continue;
            }
            if(c >= 0)
            {
                textPass.uniform("row", row);
                textPass.uniform("col", col);
                textPass.uniform("character", c);
                textPass.primitives(paz::PrimitiveType::TriangleStrip,
                    quadVertices);
            }
            ++col;
        }
        textPass.end();

        postPass.begin();
        postPass.uniform("factor", static_cast<float>(std::abs(y)));
        postPass.read("source", scenePass.framebuffer().colorAttachment(0));
        postPass.uniform("aspectRatio", static_cast<float>(paz::Window::
            AspectRatio()));
        postPass.primitives(paz::PrimitiveType::TriangleStrip, quadVertices);
        postPass.end();

        if(paz::Window::KeyPressed(paz::Key::S))
        {
            paz::write_bmp(appDir + "/screenshot.bmp", paz::Window::
                PrintScreen());
        }
    });
}
