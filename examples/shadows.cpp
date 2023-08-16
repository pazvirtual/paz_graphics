#include "PAZ_Graphics"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338328
#endif

static constexpr float ZNear = 0.1f;
static constexpr float ZFar = 100.f;
static constexpr float YFov = 65.f*M_PI/180.f;
static constexpr int Res = 2000;

static constexpr std::array<float, 4*4> GroundPos =
{
     2, -2, -2, 1,
     2,  2, -2, 1,
    -2, -2, -2, 1,
    -2,  2, -2, 1
};

static constexpr std::array<float, 4*4> GroundNor =
{
    0, 0, 1, 0,
    0, 0, 1, 0,
    0, 0, 1, 0,
    0, 0, 1, 0
};

#define P1  0.5, -0.5, -2, 1,
#define P2  0.5, -0.5, -1, 1,
#define P3 -0.5, -0.5, -1, 1,
#define P4 -0.5, -0.5, -2, 1,
#define P5  0.5,  0.5, -2, 1,
#define P6  0.5,  0.5, -1, 1,
#define P7 -0.5,  0.5, -1, 1,
#define P8 -0.5,  0.5, -2, 1,

static constexpr std::array<float, 4*3*2*6> CubePos =
{
    P3 P4 P1
    P8 P7 P6
    P5 P6 P2
    P6 P7 P3
    P8 P4 P3
    P1 P4 P8
    P3 P1 P2
    P8 P6 P5
    P5 P2 P1
    P6 P3 P2
    P8 P3 P7
    P1 P8 P5
};

#define N1  1,  0,  0, 0, //TEMP
#define N2 -1,  0,  0, 0, //TEMP
#define N3  0,  1,  0, 0, //TEMP
#define N4  0, -1,  0, 0, //TEMP
#define N5  0,  0,  1, 0, //TEMP
#define N6  0,  0, -1, 0, //TEMP

#define NN 0, 0, 0, 0, //TEMP

static constexpr std::array<float, 4*3*2*6> CubeNor =
{
    N4 N4 N4
    N3 N3 N3
    N1 N1 N1
    N5 N5 N5
    N2 N2 N2
    N6 N6 N6
    N4 N4 N4
    N3 N3 N3
    N1 N1 N1
    N5 N5 N5
    N2 N2 N2
    N6 N6 N6
};

static const std::string ShadowVertSrc = 1 + R"===(
layout(location = 0) in vec4 position;
uniform mat4 lightProjection;
uniform mat4 lightView;
void main()
{
    gl_Position = lightProjection*lightView*position;
}
)===";

static const std::string ShadowFragSrc = 1 + R"===(
void main()
{
    // Do nothing.
}
)===";

static const std::string SceneVertSrc = 1 + R"===(
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
uniform mat4 lightProjection;
uniform mat4 lightView;
uniform mat4 projection;
uniform mat4 view;
out vec4 lightProjPos;
out vec4 lightSpcNor;
void main()
{
    lightProjPos = lightProjection*lightView*position;
    lightSpcNor = lightView*normal;
    gl_Position = projection*view*position;
}
)===";

static const std::string SceneFragSrc = 1 + R"===(
in vec4 lightProjPos;
in vec4 lightSpcNor;
uniform depthSampler2D shadowMap;
layout(location = 0) out vec4 color;
void main()
{
    float ill = max(0., lightSpcNor.z/length(lightSpcNor));
    vec3 projCoords = 0.5*lightProjPos.xyz/lightProjPos.w + 0.5;
    float depth = projCoords.z;
    vec2 uv = projCoords.xy;
    if(depth - 1e-6 > texture(shadowMap, uv).r)
    {
        color = vec4(0.);
    }
    else
    {
        color = vec4(0.5*ill/length(uv));
    }
    color += 0.1;
    color.rgb = pow(clamp(color.rgb, vec3(0.), vec3(1.)), vec3(0.4545));
}
)===";

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    paz::VertexBuffer groundVerts;
    groundVerts.attribute(4, GroundPos);
    groundVerts.attribute(4, GroundNor);

    paz::VertexBuffer cubeVerts;
    cubeVerts.attribute(4, CubePos);
    cubeVerts.attribute(4, CubeNor);

    paz::RenderTarget shadowMap(Res, Res, paz::TextureFormat::Depth32Float,
        paz::MinMagFilter::Linear, paz::MinMagFilter::Linear);

    paz::Framebuffer framebuffer;
    framebuffer.attach(shadowMap);

    paz::ShaderFunctionLibrary lib;
    lib.vertex("shadow", ShadowVertSrc);
    lib.fragment("shadow", ShadowFragSrc);
    lib.vertex("scene", SceneVertSrc);
    lib.fragment("scene", SceneFragSrc);

    const paz::Shader shadow(lib, "shadow", lib, "shadow");
    const paz::Shader render(lib, "scene", lib, "scene");

    paz::RenderPass calcShadows(framebuffer, shadow);
    paz::RenderPass renderScene(render);

    const float c1 = std::cos(0.6);
    const float s1 = std::sin(0.6);
    const std::array<float, 16> lightView = {1,                  0,                  0, 0,
                                             0,                 c1,                 s1, 0,
                                             0,                -s1,                 c1, 0,
                                             0, -2.5f*c1 + 0.5f*s1, -2.5f*s1 - 0.5f*c1, 1};
    const auto lightProjection = paz::perspective(YFov, paz::Window::
        AspectRatio(), ZNear, ZFar);

    calcShadows.begin({}, paz::LoadAction::Clear);
    calcShadows.depth(paz::DepthTestMode::Less);
    calcShadows.uniform("lightView", lightView);
    calcShadows.uniform("lightProjection", lightProjection);
    calcShadows.primitives(paz::PrimitiveType::TriangleStrip, groundVerts);
    calcShadows.cull(paz::CullMode::Front);
    calcShadows.primitives(paz::PrimitiveType::Triangles, cubeVerts);
    calcShadows.end();

    double time = 0.;
    paz::Window::Loop([&]()
    {
        if(paz::Window::KeyPressed(paz::Key::Q))
        {
            paz::Window::Quit();
        }

        const float c = std::cos(time);
        const float s = std::sin(time);
        const float c0 = std::cos(5.5);
        const float s0 = std::sin(5.5);
        const std::array<float, 16> view = { c, c0*s, s*s0, 0,
                                            -s, c*c0, c*s0, 0,
                                             0,  -s0,   c0, 0,
                                             0, 2*c0, 2*s0, 1};
        const auto projection = paz::perspective(YFov, paz::Window::
            AspectRatio(), ZNear, ZFar);

        renderScene.begin({paz::LoadAction::Clear}, paz::LoadAction::Clear);
        renderScene.depth(paz::DepthTestMode::Less);
        renderScene.read("shadowMap", shadowMap);
        renderScene.uniform("view", view);
        renderScene.uniform("projection", projection);
        renderScene.uniform("lightView", lightView);
        renderScene.uniform("lightProjection", lightProjection);
        renderScene.cull(paz::CullMode::Back); //TEMP
        renderScene.primitives(paz::PrimitiveType::TriangleStrip, groundVerts);
        renderScene.primitives(paz::PrimitiveType::Triangles, cubeVerts);
        renderScene.end();

        if(!paz::Window::KeyDown(paz::Key::Space))
        {
            time += paz::Window::FrameTime();
        }
    }
    );
}
