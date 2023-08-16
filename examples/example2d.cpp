#include "PAZ_Graphics"
#include "PAZ_IO"
#include <sstream>
#include <fstream>
#include <cmath>

static constexpr double Pi    = 3.14159265358979323846264338328; // M_PI
static constexpr double TwoPi = 6.28318530717958647692528676656; // 2.*M_PI

static constexpr double BaseLength = 0.25;
static constexpr std::array<float, 6> TriPosData0 =
{
             0,  0.05,
    BaseLength,     0,
             0, -0.05
};
static constexpr std::array<float, 8> TriPosData1 =
{
             0,  0.1,
    BaseLength,    0,
             0, -0.1,
             0,  0.1
};
static constexpr std::array<float, 12> TriColorData0 =
{
    1,   0, 1, 1,
    0,   1, 1, 1,
    1, 0.5, 0, 1
};
static constexpr std::array<float, 16> TriColorData1 =
{
    1, 0, 0, 1,
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

static double fract(const double n)
{
    return n - std::floor(n);
}

static double normalize_angle(const double n)
{
    return fract(n/TwoPi)*TwoPi;
}

static void init(paz::RenderPass& scenePass, paz::RenderPass& textPass, paz::
    RenderPass& postPass, const std::string& appDir)
{
    paz::RenderTarget render(paz::TextureFormat::RGBA16Float);
    paz::Framebuffer renderFramebuffer;
    renderFramebuffer.attach(render);

    const paz::VertexFunction sceneVert(paz::read_bytes(appDir +
        "/shader.vert").str());
    const paz::VertexFunction fontVert(paz::read_bytes(appDir + "/font.vert").
        str());
    const paz::VertexFunction quadVert(paz::read_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction sceneFrag(paz::read_bytes(appDir +
        "/shader.frag").str());
    const paz::FragmentFunction fontFrag(paz::read_bytes(appDir + "/font.frag").
        str());
    const paz::FragmentFunction postFrag(paz::read_bytes(appDir + "/post.frag").
        str());

    scenePass = paz::RenderPass(renderFramebuffer, sceneVert, sceneFrag);
    textPass = paz::RenderPass(renderFramebuffer, fontVert, fontFrag, {paz::
        BlendMode::One_One});
    postPass = paz::RenderPass(quadVert, postFrag);
}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    const std::array<std::string, 2> msg = {paz::read_bytes(appDir +
        "/msg.txt").str(), paz::read_bytes(appDir + "/msg-gp.txt").str()};

    paz::Window::SetTitle("PAZ_Graphics: example2d");
    paz::Window::SetMinSize(640, 480);
    paz::Window::SetMaxSize(800, 800);

    const paz::Texture font(paz::parse_pbm(paz::read_bytes(appDir + "/font.pbm"
        )));

    paz::RenderPass scenePass, textPass, postPass;
    init(scenePass, textPass, postPass, appDir);

    paz::VertexBuffer triVertices0;
    triVertices0.addAttribute(2, TriPosData0);
    triVertices0.addAttribute(4, TriColorData0);

    paz::VertexBuffer triVertices1;
    triVertices1.addAttribute(2, TriPosData1);
    triVertices1.addAttribute(4, TriColorData1);

    paz::VertexBuffer quadVertices;
    quadVertices.addAttribute(2, QuadPosData);

    double x = 0.;
    double y = 0.;

    double angle = 0.;
    double length = 1.;

    unsigned int mode = 0;

    std::pair<double, double> cursorPos = {};

    bool hidpiEnabled = true;

    while(!paz::Window::Done())
    {
        // Handle events.
        paz::Window::PollEvents();
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
        if(paz::Window::MousePressed(0) || paz::Window::GamepadPressed(paz::
            GamepadButton::RightThumb))
        {
            mode = !mode;
        }
        if(paz::Window::KeyPressed(paz::Key::D) || paz::Window::GamepadPressed(
            paz::GamepadButton::B))
        {
            hidpiEnabled ? paz::Window::DisableHidpi() : paz::Window::
                EnableHidpi();
            hidpiEnabled = !hidpiEnabled;
        }
        if(paz::Window::GamepadActive())
        {
            cursorPos.first = std::max(0., std::min(static_cast<double>(paz::
                Window::Width()), cursorPos.first) + 3e3*paz::Window::
                GamepadRightStick().first*paz::Window::FrameTime());
            cursorPos.second = std::max(0., std::min(static_cast<double>(paz::
                Window::Height()), cursorPos.second) - 3e3*paz::Window::
                GamepadRightStick().second*paz::Window::FrameTime());
        }
        else
        {
            cursorPos = paz::Window::MousePos();
        }

        // Propagate physics.
        auto m = cursorPos;
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
            const double delta = normalize_angle(t - angle + Pi) - Pi;
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
            scenePass.draw(paz::PrimitiveType::LineStrip, triVertices1);
        }
        else
        {
            scenePass.draw(paz::PrimitiveType::Triangles, triVertices0);
        }
        scenePass.end();

        textPass.begin({paz::LoadAction::Load}, paz::LoadAction::DontCare);
        textPass.read("font", font);
        textPass.uniform("baseSize", font.height());
        textPass.uniform("aspectRatio", paz::Window::AspectRatio());
        int row = 0;
        int col = 0;
        std::stringstream fullMsg;
        fullMsg << msg[paz::Window::GamepadActive()] << std::round(1./paz::
            Window::FrameTime()) << " fps";
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
            else if(n == '/')
            {
                c = 40;
            }
            else if(n == '@')
            {
                c = 42;
            }
            else if(n == '#')
            {
                c = 43;
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
                textPass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            }
            ++col;
        }
        textPass.end();

        postPass.begin();
        postPass.uniform("factor", static_cast<float>(std::abs(y)));
        postPass.read("source", scenePass.framebuffer().colorAttachment(0));
        postPass.uniform("aspectRatio", static_cast<float>(paz::Window::
            AspectRatio()));
        postPass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
        postPass.end();

        paz::Window::EndFrame();

        if(paz::Window::KeyPressed(paz::Key::S) || paz::Window::GamepadPressed(
            paz::GamepadButton::A))
        {
            paz::write_bytes(appDir + "/screenshot.bmp", paz::to_bmp(paz::
                Window::ReadPixels()));
        }
    }
}
