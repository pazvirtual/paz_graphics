#include "PAZ_Graphics"
#include "PAZ_IO"
#include <cmath>

static constexpr double Pi = 3.14159265358979323846264338328; // M_PI

static constexpr float ZNear = 1.;
static constexpr float ZFar = 10.;
static constexpr float YFov = 65.*Pi/180.;
static constexpr int Res = 2000;
static constexpr int Size = 16;
static constexpr int Scale = 16;
static constexpr float Eps = 1e-4;

static constexpr std::array<float, 3> InstanceOffsets = {-4.5f, 0.f, 4.5f};

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

static constexpr std::array<float, 2*4> GroundUv = {4.f - Eps, Eps, 4.f - Eps,
    4.f - Eps, Eps, Eps, Eps, 4.f - Eps};

#define P1  0.5 + Eps, -0.5 - Eps, -2, 1,
#define P2  0.5 + Eps, -0.5 - Eps, -1, 1,
#define P3 -0.5 - Eps, -0.5 - Eps, -1, 1,
#define P4 -0.5 - Eps, -0.5 - Eps, -2, 1,
#define P5  0.5 + Eps,  0.5 + Eps, -2, 1,
#define P6  0.5 + Eps,  0.5 + Eps, -1, 1,
#define P7 -0.5 - Eps,  0.5 + Eps, -1, 1,
#define P8 -0.5 - Eps,  0.5 + Eps, -2, 1,

static constexpr std::array<float, 4*3*2*6> CubePos =
{
    P5 P6 P2
    P5 P2 P1
    P8 P4 P3
    P8 P3 P7
    P8 P7 P6
    P8 P6 P5
    P3 P4 P1
    P3 P1 P2
    P6 P7 P3
    P6 P3 P2
    P1 P4 P8
    P1 P8 P5
};

#define N1  1,  0,  0, 0,
#define N2 -1,  0,  0, 0,
#define N3  0,  1,  0, 0,
#define N4  0, -1,  0, 0,
#define N5  0,  0,  1, 0,
#define N6  0,  0, -1, 0,

static constexpr std::array<float, 4*3*2*6> CubeNor =
{
    N1 N1 N1
    N1 N1 N1
    N2 N2 N2
    N2 N2 N2
    N3 N3 N3
    N3 N3 N3
    N4 N4 N4
    N4 N4 N4
    N5 N5 N5
    N5 N5 N5
    N6 N6 N6
    N6 N6 N6
};

#define UV Eps, Eps, Eps, 1.f - Eps, 1.f - Eps, 1.f - Eps, Eps, Eps, 1.f - Eps,\
    1.f - Eps, 1.f - Eps, Eps,

static constexpr std::array<float, 2*3*2*6> CubeUv = {UV UV UV UV UV UV};

static const std::string ShadowVertSrc = 1 + R"===(
layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexNormal;
layout(location = 2) in vec2 vertexUv;
layout(location = 3) in float xOffset [[instance]];
uniform mat4 lightProjection;
uniform mat4 lightView;
void main()
{
    vec4 pos = vertexPosition + vec4(xOffset, 0, 0, 0);
    gl_Position = mul(lightProjection, mul(lightView, pos));
}
)===";

static const std::string ShadowFragSrc = 1 + R"===(
void main()
{
    // Do nothing.
}
)===";

static const std::string SceneVertSrc = 1 + R"===(
layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexNormal;
layout(location = 2) in vec2 vertexUv;
layout(location = 3) in float xOffset [[instance]];
uniform mat4 lightProjection;
uniform mat4 lightView;
uniform mat4 projection;
uniform mat4 view;
out vec4 lightProjPos;
out vec4 lightSpcNor;
out vec2 uv;
void main()
{
    vec4 pos = vertexPosition + vec4(xOffset, 0, 0, 0);
    lightProjPos = mul(lightProjection, mul(lightView, pos));
    lightSpcNor = mul(lightView, vertexNormal);
    gl_Position = mul(projection, mul(view, pos));
    uv = vertexUv;
}
)===";

static const std::string SceneFragSrc = 1 + R"===(
in vec4 lightProjPos;
in vec4 lightSpcNor;
in vec2 uv;
uniform depthSampler2D shadowMap;
uniform sampler2D surface;
layout(location = 0) out vec4 color;
void main()
{
    float ill = max(0., lightSpcNor.z/length(lightSpcNor));
    vec3 projCoords = 0.5*lightProjPos.xyz/lightProjPos.w + 0.5;
    float depth = projCoords.z;
    vec2 shadowUv = projCoords.xy;
    float c = (max(sign(texture(shadowMap, shadowUv).r + 1e-6 - depth), 0.)*ill
        + 0.1)*texture(surface, uv).r;
    color = vec4(c, c, c, 1);
}
)===";

#define C1 0.825336 // cos(0.6)
#define S1 0.564642 // sin(0.6)
static constexpr std::array<float, 16> lightView =
{
    1,                0,                0, 0,
    0,               C1,               S1, 0,
    0,              -S1,               C1, 0,
    0, -2.5*C1 + 0.5*S1, -2.5*S1 - 0.5*C1, 1
};
static const auto lightProjection = paz::perspective(2., 1., ZNear, ZFar);

paz::Texture compute_shadow_map(const paz::VertexBuffer& groundVerts, const
    paz::VertexBuffer& cubeVerts, const paz::InstanceBuffer& instanceAttrs)
{
    paz::RenderTarget shadowMap(paz::TextureFormat::Depth32Float, Res, Res,
        paz::MinMagFilter::Linear, paz::MinMagFilter::Linear);

    paz::Framebuffer framebuffer;
    framebuffer.attach(shadowMap);

    const paz::VertexFunction shadowVert(ShadowVertSrc);
    const paz::FragmentFunction shadowFrag(ShadowFragSrc);

    paz::RenderPass calcShadows(framebuffer, shadowVert, shadowFrag);

    calcShadows.begin({}, paz::LoadAction::Clear);
    calcShadows.depth(paz::DepthTestMode::Less);
    calcShadows.uniform("lightView", lightView);
    calcShadows.uniform("lightProjection", lightProjection);
    calcShadows.draw(paz::PrimitiveType::TriangleStrip, groundVerts,
        instanceAttrs);
    calcShadows.cull(paz::CullMode::Front);
    calcShadows.draw(paz::PrimitiveType::Triangles, cubeVerts, instanceAttrs);
    calcShadows.end();

    return shadowMap;
}

int main(int, char** argv)
{
    const std::string appDir = paz::split_path(argv[0])[0];

    paz::VertexBuffer groundVerts;
    groundVerts.addAttribute(4, GroundPos);
    groundVerts.addAttribute(4, GroundNor);
    groundVerts.addAttribute(2, GroundUv);

    paz::VertexBuffer cubeVerts;
    cubeVerts.addAttribute(4, CubePos);
    cubeVerts.addAttribute(4, CubeNor);
    cubeVerts.addAttribute(2, CubeUv);

    paz::InstanceBuffer instanceAttrs(InstanceOffsets.size());
    instanceAttrs.addAttribute(1, paz::DataType::Float);
    instanceAttrs.subAttribute(0, InstanceOffsets);

    paz::VertexBuffer quadVerts;
    quadVerts.addAttribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    const paz::VertexFunction sceneVert(SceneVertSrc);
    const paz::VertexFunction quadVert(paz::read_bytes(appDir + "/quad.vert").
        str());
    const paz::FragmentFunction sceneFrag(SceneFragSrc);
    const paz::FragmentFunction aaFrag(paz::read_bytes(appDir + "/fxaa.frag").
        str());

    paz::Framebuffer buff;
    buff.attach(paz::RenderTarget(paz::TextureFormat::RGBA16Float, paz::
        MinMagFilter::Linear, paz::MinMagFilter::Linear));
    buff.attach(paz::RenderTarget(paz::TextureFormat::Depth32Float));

    paz::RenderPass scenePass(buff, sceneVert, sceneFrag);
    paz::RenderPass aaPass(quadVert, aaFrag);

    const paz::Texture shadowMap = compute_shadow_map(groundVerts, cubeVerts,
        instanceAttrs);
    paz::Image img(paz::ImageFormat::R8UNorm, Scale*Size, Scale*Size);
    for(std::size_t i = 0; i < Size; ++i)
    {
        for(std::size_t j = 0; j < Size; ++j)
        {
            const std::uint8_t c = 255*((Size*i + j + i%2)%2);
            for(std::size_t a = 0; a < Scale; ++a)
            {
                for(std::size_t b = 0; b < Scale; ++b)
                {
                    img.bytes()[img.width()*(Scale*i + a) + (Scale*j + b)] = c;
                }
            }
        }
    }
    const paz::Texture surface(img, paz::MinMagFilter::Linear, paz::
        MinMagFilter::Linear, paz::MipmapFilter::Linear, paz::WrapMode::Repeat,
        paz::WrapMode::Repeat);

    double time = 0.;
    while(!paz::Window::Done())
    {
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

        scenePass.begin({paz::LoadAction::Clear}, paz::LoadAction::Clear);
        scenePass.depth(paz::DepthTestMode::Less);
        scenePass.read("shadowMap", shadowMap);
        scenePass.read("surface", surface);
        scenePass.uniform("view", view);
        scenePass.uniform("projection", projection);
        scenePass.uniform("lightView", lightView);
        scenePass.uniform("lightProjection", lightProjection);
        scenePass.cull(paz::CullMode::Back);
        scenePass.draw(paz::PrimitiveType::TriangleStrip, groundVerts,
            instanceAttrs);
        scenePass.draw(paz::PrimitiveType::Triangles, cubeVerts, instanceAttrs);
        scenePass.end();

        aaPass.begin();
        aaPass.read("img", buff.colorAttachment(0));
        aaPass.read("lum", buff.colorAttachment(0)); // works because grayscale
        aaPass.draw(paz::PrimitiveType::TriangleStrip, quadVerts);
        aaPass.end();

        paz::Window::EndFrame();

        if(!paz::Window::KeyDown(paz::Key::Space) && !paz::Window::GamepadDown(
            paz::GamepadButton::A))
        {
            time += paz::Window::FrameTime();
        }
    }
}
