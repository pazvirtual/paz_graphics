#include "PAZ_Graphics"
#include <sstream>
#include <fstream>
#include <cmath>

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

static constexpr double l = 0.25;
static const std::vector<float> a0 =
{
    0.,  0.05,
     l,    0.,
    0., -0.05
};
static const std::vector<float> a1 =
{
    0.,  0.1,
     l,   0.,
    0., -0.1
};
static const std::vector<float> b0 =
{
    1.,  0., 1., 1.,
    0.,  1., 1., 1.,
    1., 0.5, 0., 1.
};
static const std::vector<float> b1 =
{
    1., 0., 0., 1.,
    1., 0., 0., 1.,
    1., 0., 0., 1.
};

static const std::vector<float> qv =
{
     1., -1.,
     1.,  1.,
    -1., -1.,
    -1.,  1.
};

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

static int load_font(const std::string& path, std::vector<float>& v)
{
    std::ifstream in(path);
    if(!in)
    {
        throw std::runtime_error("Unable to open input file \"" + path + "\".");
    }
    int numRows = 0;
    std::vector<float> u;
    std::string line;
    while(std::getline(in, line))
    {
        ++numRows;
        for(const auto& n : line)
        {
            u.push_back(n != ' ');
        }
    }
    const std::size_t w = u.size()/numRows;
    v.resize(u.size());
    for(int i = 0; i < numRows; ++i)
    {
        for(std::size_t j = 0; j < w; ++j)
        {
            v[w*(numRows - i - 1) + j] = u[w*i + j];
        }
    }
    return numRows;
}
//TEMP

int main()
{
    const std::string msg = read_file("msg.txt");

    paz::Window::SetMinSize(640, 480);

    std::vector<float> fontData;
    const int fontRows = load_font("font.txt", fontData);
    const paz::Texture font(fontData.size()/fontRows, fontRows, 1, 8*sizeof(
        float), fontData, paz::Texture::MinMagFilter::Nearest, paz::Texture::
        MinMagFilter::Nearest);

    paz::RenderTarget scene(1., 4, 16, paz::Texture::DataType::Float, paz::
        Texture::MinMagFilter::Linear, paz::Texture::MinMagFilter::Linear);
    paz::Framebuffer sceneFramebuffer;
    sceneFramebuffer.attach(scene);

    paz::RenderTarget overlay(1., 1, 16, paz::Texture::DataType::Float, paz::
        Texture::MinMagFilter::Linear, paz::Texture::MinMagFilter::Linear);
    paz::Framebuffer overlayFramebuffer;
    overlayFramebuffer.attach(overlay);

    paz::RenderTarget blended(1., 4, 16, paz::Texture::DataType::Float, paz::
        Texture::MinMagFilter::Linear, paz::Texture::MinMagFilter::Linear);
    paz::Framebuffer blendedFramebuffer;
    blendedFramebuffer.attach(blended);

    paz::ShaderFunctionLibrary shaders;
    shaders.vertex("shader", read_file("shader.vert"));
    shaders.vertex("font", read_file("font.vert"));
    shaders.vertex("quad", read_file("quad.vert")),
    shaders.fragment("shader", read_file("shader.frag"));
    shaders.fragment("font", read_file("font.frag"));
    shaders.fragment("blend", read_file("blend.frag"));
    shaders.fragment("post", read_file("post.frag"));

    const paz::Shader s(shaders, "shader", shaders, "shader");
    const paz::Shader f(shaders, "font", shaders, "font");
    const paz::Shader blend(shaders, "quad", shaders, "blend");
    const paz::Shader post(shaders, "quad", shaders, "post");

    paz::RenderPass r0(sceneFramebuffer, s);
    paz::RenderPass r1(overlayFramebuffer, f);
    paz::RenderPass r2(blendedFramebuffer, blend);
    paz::RenderPass r3(post);

    paz::VertexBuffer vertices0;
    vertices0.attribute(2, a0);
    vertices0.attribute(4, b0);

    paz::VertexBuffer vertices1;
    vertices1.attribute(2, a1);
    vertices1.attribute(4, b1);

    const paz::IndexBuffer loopIndices({0, 1, 2, 0});

    paz::VertexBuffer quadVertices;
    quadVertices.attribute(2, qv);

    double x = 0.;
    double y = 0.;

    double g = 0.;
    double length = 1.;

    unsigned int mode = 0;

    paz::Window::Loop([&]()
    {
        // Handle events.
        if(paz::Window::KeyPressed(paz::Window::Key::Q))
        {
            paz::Window::Quit();
        }
        if(paz::Window::KeyPressed(paz::Window::Key::F))
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
        m.first = std::min(m.first, (double)paz::Window::Width());
        m.second = std::min(m.second, (double)paz::Window::Height());
        m.first = std::max(m.first*2./paz::Window::Height(), 0.);
        m.second = std::max(m.second*2./paz::Window::Height(), 0.);
        m.first -= paz::Window::AspectRatio();
        m.second -= 1.;
        const double dX = m.first - x;
        const double dY = m.second - y;
        const double dNorm = std::sqrt(dX*dX + dY*dY);
        length = std::max(1., dNorm/l);

        // Drawing prep.
        if(dNorm > 1e-2)
        {
            const double h = std::atan2(dY, dX);
            const double delta = normalize_angle(h - g + M_PI) - M_PI;
            g = normalize_angle(g + 0.5*delta);

            const double k = std::min(10.*paz::Window::FrameTime(), 1.);
            x += k*dX;
            y += k*dY;
        }

        // Drawing.
        r0.begin({paz::RenderPass::LoadAction::Clear});
        r0.uniform("angle", (float)g);
        r0.uniform("aspectRatio", (float)paz::Window::AspectRatio());
        r0.uniform("p", (float)x, (float)y);
        r0.uniform("length", (float)length);
        if(mode)
        {
            r0.indexed(paz::RenderPass::PrimitiveType::LineStrip, vertices1,
                loopIndices);
        }
        else
        {
            r0.primitives(paz::RenderPass::PrimitiveType::Triangles, vertices0);
        }
        r0.end();

        r1.begin({paz::RenderPass::LoadAction::Clear});
        r1.read("font", font);
        r1.uniform("aspectRatio", paz::Window::AspectRatio());
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
                r1.uniform("row", row);
                r1.uniform("col", col);
                r1.uniform("character", c);
                r1.primitives(paz::RenderPass::PrimitiveType::TriangleStrip,
                    quadVertices);
            }
            ++col;
        }
        r1.end();

        r2.begin();
        r2.read("render", scene);
        r2.read("overlay", overlay);
        r2.primitives(paz::RenderPass::PrimitiveType::TriangleStrip,
            quadVertices);
        r2.end();

        r3.begin();
        r3.uniform("factor", (float)std::abs(y));
        r3.read("source", blended);
        r3.uniform("aspectRatio", (float)paz::Window::AspectRatio());
        r3.primitives(paz::RenderPass::PrimitiveType::TriangleStrip,
            quadVertices);
        r3.end();
    });
}
