#include "PAZ_Graphics"
#include "PAZ_IO"
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

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    paz::Window::SetMinSize(640, 480);

    paz::ColorTarget render(1., 4, 16, paz::Texture::DataType::Float, paz::
        Texture::MinMagFilter::Linear, paz::Texture::MinMagFilter::Linear);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    paz::ShaderFunctionLibrary l;
    l.vertex("particle", read_file(appDir + "/particle.vert")),
    l.fragment("particle", read_file(appDir + "/particle.frag"));
    l.vertex("quad", read_file(appDir + "/quad.vert")),
    l.fragment("tonemap", read_file(appDir + "/tonemap.frag"));

    const paz::Shader s(l, "particle", l, "particle");
    const paz::Shader t(l, "quad", l, "tonemap");

    paz::RenderPass r(renderFramebuffer, s, paz::RenderPass::BlendMode::
        Additive);
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
            time += paz::Window::FrameTime();
        }
        const float c = std::cos(0.1*time);
        const float s = std::sin(0.1*time);
        const std::array<float, 16> v = {c, 0, -s, 0,
                                         0, 1,  0, 0,
                                         s, 0,  c, 0,
                                         0, 0,  0, 1};

        // Drawing.
        r.begin({paz::RenderPass::LoadAction::Clear}, paz::RenderPass::
            LoadAction::DontCare);
        r.uniform("projection", p);
        r.uniform("view", v);
        for(auto it = particles.rbegin(); it != particles.rend(); ++it)
        {
            r.uniform("origin", (float)it->second[0], (float)it->second[1],
                (float)it->second[2]);
            r.uniform("distSq", (float)it->first);
            r.primitives(paz::RenderPass::PrimitiveType::TriangleStrip, q);
        }
        r.end();

        u.begin();
        u.read("hdrRender", render);
        u.uniform("whitePoint", 0.5f + 0.5f*(float)std::sin(time));
        u.primitives(paz::RenderPass::PrimitiveType::TriangleStrip, q);
        u.end();
    });
}
