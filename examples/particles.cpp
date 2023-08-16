#include "PAZ_Graphics"
#include <sstream>
#include <fstream>
#include <cmath>
#include <random>
#include <map>

static constexpr std::size_t NumParticles = 1000;
static constexpr double ZDist = 0.;
static constexpr double Side = 10.;

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

    paz::RenderTarget render(1., paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    paz::ShaderFunctionLibrary l;
    l.vertex("particle", paz::load_file(appDir + "/particle.vert").str()),
    l.fragment("particle", paz::load_file(appDir + "/particle.frag").str());
    l.vertex("quad", paz::load_file(appDir + "/quad.vert").str()),
    l.fragment("tonemap", paz::load_file(appDir + "/tonemap.frag").str());

    const paz::Shader s(l, "particle", l, "particle");
    const paz::Shader t(l, "quad", l, "tonemap");

    paz::RenderPass r(renderFramebuffer, s, paz::BlendMode::Additive);
    paz::RenderPass u(t);

    paz::VertexBuffer q;
    q.attribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    std::multimap<double, std::array<double, 3>> particles;
    for(std::size_t i = 0; i < NumParticles; ++i)
    {
        const double x = Side*2.*(uniform() - 0.5);
        const double y = Side*2.*(uniform() - 0.5);
        const double z = Side*2.*(uniform() - 0.5) - ZDist;
        const double distSq = x*x + y*y + z*z;
        particles.emplace(distSq, std::array<double, 3>{x, y, z});
    }

    double time = 0.;
    paz::Window::Loop([&]()
    {
        // Handle events.
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }

        // Propagate physics.
        // ...

        // Drawing prep.
        const std::array<float, 16> p = paz::perspective(1., paz::Window::
            AspectRatio(), 0.01f, 100.f);
        if(!paz::Window::KeyDown(paz::Key::Space))
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
        r.begin({paz::LoadAction::Clear}, paz::LoadAction::DontCare);
        r.uniform("projection", p);
        r.uniform("view", v);
        for(auto it = particles.rbegin(); it != particles.rend(); ++it)
        {
            r.uniform("origin", static_cast<float>(it->second[0]), static_cast<
                float>(it->second[1]), static_cast<float>(it->second[2]));
            r.uniform("distSq", static_cast<float>(it->first));
            r.primitives(paz::PrimitiveType::TriangleStrip, q);
        }
        r.end();

        u.begin();
        u.read("hdrRender", render);
        u.uniform("whitePoint", 0.5f + 0.5f*static_cast<float>(std::sin(time)));
        u.primitives(paz::PrimitiveType::TriangleStrip, q);
        u.end();
    });
}
