#include "PAZ_Graphics"
#include <sstream>
#include <fstream>
#include <cmath>

static constexpr double BaseLength = 0.25;
static const std::vector<float> TriPosData0 =
{
             0,  0.05,
    BaseLength,     0,
             0, -0.05
};
static const std::vector<float> TriPosData1 =
{
             0,  0.1,
    BaseLength,    0,
             0, -0.1
};
static const std::vector<float> TriColorData0 =
{
    1,   0, 1, 1,
    0,   1, 1, 1,
    1, 0.5, 0, 1
};
static const std::vector<float> TriColorData1 =
{
    1, 0, 0, 1,
    1, 0, 0, 1,
    1, 0, 0, 1
};

static const std::vector<float> QuadPosData =
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

//TEMP
void write_bmp(const std::string& path, unsigned int width, const std::vector<
    float>& data)
{
    unsigned int extraBytes = 4 - ((3*width)%4);
    if(extraBytes == 4)
    {
        extraBytes = 0;
    }

    const unsigned int height = data.size()/(3*width);
    const unsigned int paddedSize = (3*width + extraBytes)*height;
    const std::array<unsigned int, 13> headers =
    {
        paddedSize + 54, 0, 54, 40, width, height, 0, 0, paddedSize, 0, 0, 0, 0
    };

    std::ofstream out(path, std::ios::binary);
    if(!out)
    {
        throw std::runtime_error("Unable to open \"" + path + "\".");
    }

    out << "BM";

    for(int i = 0; i < 6; ++i)
    {
        out.put(headers[i]&0x000000ff);
        out.put((headers[i]&0x0000ff00) >> 8);
        out.put((headers[i]&0x00ff0000) >> 16);
        out.put((headers[i]&(unsigned int)0xff000000) >> 24);
    }

    out.put(1);
    out.put(0);
    out.put(24);
    out.put(0);

    for(int i = 7; i < 13; ++i)
    {
        out.put(headers[i]&0x000000ff);
        out.put((headers[i]&0x0000ff00) >> 8);
        out.put((headers[i]&0x00ff0000) >> 16);
        out.put((headers[i]&(unsigned int)0xff000000) >> 24);
    }

    for(unsigned int y = 0; y < height; ++y)
    {
        for(unsigned int x = 0; x < width; ++x)
        {
            for(int i = 2; i >= 0; --i)
            {
                out.put(std::min((unsigned int)std::round(std::max(data[3*(y*
                    width + x) + i], 0.f)*255.f), 255u));
            }
        }
        for(unsigned int i = 0; i < extraBytes; ++i)
        {
            out.put(0);
        }
    }
}

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

static unsigned int load_font(const std::string& path, std::vector<float>& data)
{
    std::ifstream in(path);
    if(!in)
    {
        throw std::runtime_error("Unable to open input file \"" + path + "\".");
    }
    unsigned int height = 0;
    std::vector<float> v;
    std::string line;
    while(std::getline(in, line))
    {
        ++height;
        for(const auto& n : line)
        {
            v.push_back(n != ' ');
        }
    }
    const std::size_t width = v.size()/height;
    data.resize(v.size());
    for(unsigned int i = 0; i < height; ++i)
    {
        for(std::size_t j = 0; j < width; ++j)
        {
            data[width*(height - i - 1) + j] = v[width*i + j];
        }
    }
    return height;
}
//TEMP

int main()
{
    const std::string msg = read_file("msg.txt");

    paz::Window::SetMinSize(640, 480);

    std::vector<float> fontData;
    const unsigned int fontRows = load_font("font.txt", fontData);
    const paz::Texture font(fontData.size()/fontRows, fontRows, 1, 8*sizeof(
        float), fontData, paz::Texture::MinMagFilter::Nearest, paz::Texture::
        MinMagFilter::Nearest);

    paz::ColorTarget render(1., 4, 16, paz::Texture::DataType::Float, paz::
        Texture::MinMagFilter::Linear, paz::Texture::MinMagFilter::Linear);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    paz::ShaderFunctionLibrary shaders;
    shaders.vertex("shader", read_file("shader.vert"));
    shaders.vertex("font", read_file("font.vert"));
    shaders.vertex("quad", read_file("quad.vert")),
    shaders.fragment("shader", read_file("shader.frag"));
    shaders.fragment("font", read_file("font.frag"));
    shaders.fragment("post", read_file("post.frag"));

    const paz::Shader sceneShader(shaders, "shader", shaders, "shader");
    const paz::Shader textShader(shaders, "font", shaders, "font");
    const paz::Shader postShader(shaders, "quad", shaders, "post");

    paz::RenderPass scenePass(renderFramebuffer, sceneShader);
    paz::RenderPass textPass(renderFramebuffer, textShader);
    paz::RenderPass postPass(postShader);

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
        scenePass.begin({paz::RenderPass::LoadAction::Clear});
        scenePass.uniform("angle", (float)angle);
        scenePass.uniform("aspectRatio", (float)paz::Window::AspectRatio());
        scenePass.uniform("origin", (float)x, (float)y);
        scenePass.uniform("length", (float)length);
        if(mode)
        {
            scenePass.indexed(paz::RenderPass::PrimitiveType::LineStrip,
                triVertices1, loopIndices);
        }
        else
        {
            scenePass.primitives(paz::RenderPass::PrimitiveType::Triangles,
                triVertices0);
        }
        scenePass.end();

        textPass.begin({paz::RenderPass::LoadAction::Load}, paz::RenderPass::
            LoadAction::DontCare, paz::RenderPass::BlendMode::Additive);
        textPass.read("font", font);
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
                textPass.primitives(paz::RenderPass::PrimitiveType::
                    TriangleStrip, quadVertices);
            }
            ++col;
        }
        textPass.end();

        postPass.begin();
        postPass.uniform("factor", (float)std::abs(y));
        postPass.read("source", render);
        postPass.uniform("aspectRatio", (float)paz::Window::AspectRatio());
        postPass.primitives(paz::RenderPass::PrimitiveType::TriangleStrip,
            quadVertices);
        postPass.end();

        if(paz::Window::KeyPressed(paz::Window::Key::S))
        {
            write_bmp("screenshot.bmp", paz::Window::ViewportWidth(), paz::
                Window::PrintScreen());
        }
    });
}
