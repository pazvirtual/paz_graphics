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

//TEMP
static std::string read_file(const std::string& path)
{
    std::ifstream in(path);
    if(!in)
    {
        throw std::runtime_error("Unable to open input file \"" + path + "\".");
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
//TEMP

int main()
{
    paz::Window::SetMinSize(640, 480);

    paz::ShaderFunctionLibrary l;
    l.vertex("particle", read_file("particle.vert")),
    l.fragment("particle", read_file("particle.frag"));

    const paz::Shader s(l, "particle", l, "particle");

    paz::RenderPass r(s);

    paz::VertexBuffer q;
    q.attribute(2, std::vector<float>{1., -1., 1., 1., -1., -1., -1., 1.});

    std::multimap<double, std::array<double, 4>> particles;
    for(std::size_t i = 0; i < NumParticles; ++i)
    {
        const double x = Side*2.*(uniform() - 0.5);
        const double y = Side*2.*(uniform() - 0.5);
        const double z = Side*2.*(uniform() - 0.5) - ZDist;
        const double size = uniform();
        const double distSq = x*x + y*y + z*z;
        particles.emplace(distSq, std::array<double, 4>{x, y, z, size});
    }

    double t = 0.;
    paz::Window::Loop([&]()
    {
        // Handle events.
        if(paz::Window::KeyPressed(paz::Window::Key::Q))
        {
            paz::Window::Quit();
        }

        // Propagate physics.
        // ...

        // Drawing prep.
        const std::array<float, 16> p = paz::perspective(1., paz::Window::
            AspectRatio(), 0.01f, 100.f);
        if(!paz::Window::KeyDown(paz::Window::Key::Space))
        {
            t += paz::Window::FrameTime();
        }
        const float c = std::cos(0.1*t);
        const float s = std::sin(0.1*t);
        const std::array<float, 16> v = {c, 0, -s, 0,
                                         0, 1,  0, 0,
                                         s, 0,  c, 0,
                                         0, 0,  0, 1};

        // Drawing.
        r.begin({paz::RenderPass::LoadAction::Clear});
        r.blend(paz::RenderPass::BlendMode::Additive);
        r.uniform("projection", p.data(), 16);
        r.uniform("view", v.data(), 16);
        for(auto it = particles.rbegin(); it != particles.rend(); ++it)
        {
            r.uniform("origin", (float)it->second[0], (float)it->second[1],
                (float)it->second[2]);
            r.uniform("size", (float)it->second[3]);
            r.uniform("distSq", (float)it->first);
            r.primitives(paz::RenderPass::PrimitiveType::TriangleStrip, q);
        }
        r.blend(paz::RenderPass::BlendMode::Disable);//TEMP ?
        r.end();
    });
}
