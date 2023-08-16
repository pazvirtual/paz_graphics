#include "PAZ_Graphics"
#include "PAZ_IO"
#include <sstream>
#include <fstream>
#include <cmath>
#include <random>
#include <map>

static constexpr std::size_t NumParticles = 2000;
static constexpr double ZDist = 0.;
static constexpr double Side = 10.;
static constexpr double MinDist = 1.;
static constexpr float MaxLum = 2.f;

thread_local std::mt19937_64 RandomEngine(0);

static double uniform()
{
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(RandomEngine);
}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    paz::Window::SetMinSize(640, 480);

    paz::RenderTarget render(paz::TextureFormat::RGBA16Float);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    const paz::VertexFunction particleVert(paz::read_bytes(appDir +
        "/particle.vert").str());
    const paz::FragmentFunction particleFrag(paz::read_bytes(appDir +
        "/particle.frag").str());
    const paz::VertexFunction quad(paz::read_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction tonemap(paz::read_bytes(appDir +
        "/tonemap.frag").str());

    paz::RenderPass r(renderFramebuffer, particleVert, particleFrag, paz::
        BlendMode::Blend);
    paz::RenderPass u(quad, tonemap);

    paz::VertexBuffer q;
    q.addAttribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    paz::InstanceBuffer instances;
    {
        std::multimap<double, std::array<double, 3>> particles;
        for(std::size_t i = 0; i < NumParticles; ++i)
        {
            double x = 0.;
            double y = 0.;
            double z = 0.;
            double distSq = 0.;
            while(distSq < MinDist*MinDist)
            {
                x = Side*2.*(uniform() - 0.5);
                y = Side*2.*(uniform() - 0.5);
                z = Side*2.*(uniform() - 0.5) - ZDist;
                distSq = x*x + y*y + z*z;
            }
            particles.emplace(distSq, std::array<double, 3>{x, y, z});
        }

        std::vector<float> origins;
        std::vector<float> colors;
        origins.reserve(4*NumParticles);
        colors.reserve(4*NumParticles);
        for(auto it = particles.rbegin(); it != particles.rend(); ++it)
        {
            for(int i = 0; i < 3; ++i)
            {
                origins.push_back(it->second[i]);
            }
            origins.push_back(1);
            for(int i = 0; i < 3; ++i)
            {
                colors.push_back(MaxLum*uniform());
            }
            colors.push_back(1);
        }

        instances.addAttribute(4, origins);
        instances.addAttribute(4, colors);
    }

    double time = 0.;
    while(!paz::Window::Done())
    {
        // Handle events.
        paz::Window::SetCursorMode(paz::Window::GamepadActive() ? paz::
            CursorMode::Disable : paz::CursorMode::Normal);
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

        // Propagate physics.
        // ...

        // Drawing prep.
        const std::array<float, 16> p = paz::perspective(1., paz::Window::
            AspectRatio(), 0.01f, 100.f);
        if(!paz::Window::KeyDown(paz::Key::Space) && !paz::Window::GamepadDown(
            paz::GamepadButton::A))
        {
            time += paz::Window::FrameTime();
        }
        const float c = std::cos(0.1*time);
        const float s = std::sin(0.1*time);
        const std::array<float, 16> v = {c, 0, -s, 0,
                                         0, 1,  0, 0,
                                         s, 0,  c, 0,
                                         0, 0,  0, 1};

        // Drawing.
        r.begin({paz::LoadAction::FillOnes});
        r.uniform("projection", p);
        r.uniform("view", v);
        r.draw(paz::PrimitiveType::TriangleStrip, q, instances);
        r.end();

        u.begin();
        u.read("hdrRender", render);
        u.uniform("whitePoint", 1.2f*MaxLum);
        u.draw(paz::PrimitiveType::TriangleStrip, q);
        u.end();

        paz::Window::EndFrame();
    }
}
