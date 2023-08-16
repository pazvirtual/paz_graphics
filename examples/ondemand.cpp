#include "PAZ_Graphics"
#include "PAZ_IO"
#include <sstream>
#include <iomanip>

static constexpr std::size_t NumSteps = 100;
static constexpr std::size_t Res = 500;

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];
    paz::Window::MakeNotResizable();
    paz::Window::Resize(Res, Res, true);
    if(paz::Window::ViewportWidth() != Res || paz::Window::ViewportHeight() !=
        Res)
    {
        std::cerr << "Error: Failed to resize to correct dimensions." << std::
            endl;
        return 1;
    }

    paz::VertexBuffer quadVertices;
    quadVertices.attribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    const paz::Texture font(paz::parse_pbm(paz::load_bytes(appDir + "/font.pbm"
        )));

    const paz::VertexFunction fontVert(paz::load_bytes(appDir + "/font.vert").
        str());
    const paz::FragmentFunction fontFrag(paz::load_bytes(appDir + "/font.frag").
        str());

    paz::RenderPass textPass(fontVert, fontFrag);

    double time = 0.;
    for(std::size_t k = 0; k < NumSteps; ++k)
    {
        std::string str;
        {
            std::stringstream ss;
            ss << std::fixed << std::setw(4) << std::setprecision(1) << time <<
                " " << std::setw(4) << k + 1 << "/" << NumSteps;
            str = ss.str();
        }
        textPass.begin({paz::LoadAction::Clear});
        textPass.uniform("baseSize", 2*font.height());
        textPass.uniform("aspectRatio", paz::Window::AspectRatio());
        textPass.read("font", font);
        for(std::size_t i = 0; i < str.size(); ++i)
        {
            char c = -1;
            if(str[i] >= '0' && str[i] <= '9')
            {
                c = str[i] - '0';
            }
            else if(str[i] == '.')
            {
                c = 37;
            }
            else if(str[i] == '/')
            {
                c = 40;
            }
            if(c >= 0)
            {
                textPass.uniform("row", 0);
                textPass.uniform("col", static_cast<int>(i));
                textPass.uniform("character", c);
                textPass.draw(paz::PrimitiveType::TriangleStrip, quadVertices);
            }
        }
        textPass.end();
        paz::Window::EndFrame();
        time += paz::Window::FrameTime();
    }
}
