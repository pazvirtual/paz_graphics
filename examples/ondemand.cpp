#include "PAZ_Graphics"

static constexpr std::size_t NumSteps = 1000;

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];
    paz::Window::MakeNotResizable();
    paz::Window::Resize(1000, 1000);

    paz::VertexBuffer quadVertices;
    quadVertices.attribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    const paz::Texture font = paz::Texture(paz::load_pbm(appDir + "/font.pbm"));

    paz::ShaderFunctionLibrary lib;
    lib.vertex("font", paz::load_file(appDir + "/font.vert").str());
    lib.fragment("font", paz::load_file(appDir + "/font.frag").str());

    paz::RenderPass textPass(paz::Shader(lib, "font", lib, "font"));

    double time = 0.;
    for(std::size_t k = 0; k < NumSteps; ++k)
    {
        const std::string str = std::to_string(time) + " " + std::to_string(k +
            1) + "/" + std::to_string(NumSteps);
        textPass.begin({paz::LoadAction::Clear});
        textPass.uniform("baseSize", font.height());
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
                textPass.primitives(paz::PrimitiveType::TriangleStrip,
                    quadVertices);
            }
        }
        textPass.end();
        paz::Window::Commit();
        time += paz::Window::FrameTime();
    }
}
