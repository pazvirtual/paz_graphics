#include "PAZ_Graphics"
#include <cmath>
#include <iomanip>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338328
#endif

#define CATCH \
    catch(const std::exception& e) \
    { \
        std::cerr << "Failed " << test << ": " << e.what() << std::endl; \
        return 1; \
    } \
    std::cout << "Passed " << test << std::endl; \
    ++test;

#define EXPECT_EXCEPTION(statement) \
    try \
    { \
        statement; \
        std::cerr << "Failed " << test << ": Did not throw." << std::endl; \
        return 1; \
    } \
    catch(const std::exception& e) \
    { \
        std::cerr << "Passed " << test << std::endl; \
    } \
    ++test;

static constexpr float ZNear = 1.;
static constexpr float ZFar = 5.;
static constexpr float YFov = 65.*M_PI/180.;
// Texture widths are odd to verify robustness.
static constexpr int ShadowRes = 1999;
// Setting window dimensions to odd numbers of physical pixels will cause test
// to fail on HiDPI displays.
static constexpr int ImgRes = 200;
static constexpr int Size = 16;
static constexpr int Scale = 8;
static constexpr float Eps = 1e-4;
static constexpr double Angle = 3.;

static constexpr int Threshold = 0.02*std::numeric_limits<std::uint8_t>::max();

static constexpr std::array<std::array<int, 3>, 10> SamplePoints =
{{
    {171,  83, 0},
    {115, 158, 111},
    {  3,  59, 0},
    { 62,  69, 52},
    {100, 162, 191},
    { 97, 130, 53},
    {189,  70, 0},
    {138,  37, 183},
    { 75,  70, 131},
    {134,  64, 156}
}};

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
uniform mat4 lightProjection;
uniform mat4 lightView;
void main()
{
    gl_Position = lightProjection*lightView*vertexPosition;
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
uniform mat4 lightProjection;
uniform mat4 lightView;
uniform mat4 projection;
uniform mat4 view;
out vec4 lightProjPos;
out vec4 lightSpcNor;
out vec2 uv;
void main()
{
    lightProjPos = lightProjection*lightView*vertexPosition;
    lightSpcNor = lightView*vertexNormal;
    gl_Position = projection*view*vertexPosition;
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
    color = vec4((max(sign(texture(shadowMap, shadowUv).r + 1e-6 - depth), 0.)*
        ill + 0.1)*texture(surface, uv).r);
    color.rgb = clamp(color.rgb, vec3(0.), vec3(1.));
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
static const auto lightProjection = paz::perspective(YFov, 1., ZNear, ZFar);

paz::Texture compute_shadow_map(const paz::VertexBuffer& groundVerts, const
    paz::VertexBuffer& cubeVerts)
{
    paz::RenderTarget shadowMap(paz::TextureFormat::Depth32Float, ShadowRes,
        ShadowRes, paz::MinMagFilter::Linear, paz::MinMagFilter::Linear);

    paz::Framebuffer framebuffer;
    framebuffer.attach(shadowMap);

    const paz::VertexFunction shadowVert(ShadowVertSrc);
    const paz::FragmentFunction shadowFrag(ShadowFragSrc);

    paz::RenderPass calcShadows(framebuffer, shadowVert, shadowFrag);

    paz::VertexBuffer quadVerts;
    quadVerts.addAttribute(2, std::array<float, 8>{1, -1, 1, 1, -1, -1, -1, 1});

    calcShadows.begin({}, paz::LoadAction::Clear);
    calcShadows.depth(paz::DepthTestMode::Less);
    calcShadows.uniform("lightView", lightView);
    calcShadows.uniform("lightProjection", lightProjection);
    calcShadows.draw(paz::PrimitiveType::TriangleStrip, groundVerts);
    calcShadows.cull(paz::CullMode::Front);
    calcShadows.draw(paz::PrimitiveType::Triangles, cubeVerts);
    calcShadows.end();

    return shadowMap;
}

int main()
{
    int test = 1;

    try
    {
        paz::Window::MakeNotResizable();
    }
    CATCH

    try
    {
        paz::Window::Resize(ImgRes, ImgRes, true);
        if(paz::Window::ViewportWidth() != ImgRes || paz::Window::
            ViewportHeight() != ImgRes)
        {
            throw std::runtime_error("Failed to resize to correct dimensions.");
        }
    }
    CATCH

    paz::VertexBuffer groundVerts;
    paz::VertexBuffer cubeVerts;
    try
    {
        groundVerts.addAttribute(4, GroundPos);
        groundVerts.addAttribute(4, GroundNor);
        groundVerts.addAttribute(2, GroundUv);

        cubeVerts.addAttribute(4, CubePos);
        cubeVerts.addAttribute(4, CubeNor);
        cubeVerts.addAttribute(2, CubeUv);
    }
    CATCH

    paz::RenderPass scenePass;
    paz::RenderPass otherPass;
    try
    {
        const paz::VertexFunction sceneVert(SceneVertSrc);
        const paz::FragmentFunction sceneFrag(SceneFragSrc);
        scenePass = paz::RenderPass(sceneVert, sceneFrag);
        otherPass = paz::RenderPass(sceneVert, sceneFrag);
    }
    CATCH

    paz::Texture shadowMap;
    try
    {
        shadowMap = compute_shadow_map(groundVerts, cubeVerts);
    }
    CATCH

    paz::Texture surface;
    try
    {
        // Texture width is odd again to verify robustness.
        paz::Image img(paz::ImageFormat::R8UNorm, Scale*Size - 1, Scale*Size);
        for(std::size_t i = 0; i < Size; ++i)
        {
            for(std::size_t j = 0; j < Size; ++j)
            {
                const std::uint8_t c = 255*((Size*i + j + i%2)%2);
                for(std::size_t a = 0; a < Scale; ++a)
                {
                    for(std::size_t b = 0; b < Scale; ++b)
                    {
                        if(static_cast<int>(Scale*j + b) == img.width())
                        {
                            break;
                        }
                        img.bytes()[img.width()*(Scale*i + a) + (Scale*j + b)] =
                            c;
                    }
                }
            }
        }
        surface = paz::Texture(img, paz::MinMagFilter::Linear, paz::
            MinMagFilter::Nearest, paz::MipmapFilter::Linear, paz::WrapMode::
            Repeat, paz::WrapMode::Repeat);
    }
    CATCH

    try
    {
        const float c = std::cos(Angle);
        const float s = std::sin(Angle);
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
        EXPECT_EXCEPTION(otherPass.uniform("projection", projection))
        scenePass.uniform("lightView", lightView);
        scenePass.uniform("lightProjection", lightProjection);
        scenePass.cull(paz::CullMode::Back);
        scenePass.draw(paz::PrimitiveType::TriangleStrip, groundVerts);
        scenePass.draw(paz::PrimitiveType::Triangles, cubeVerts);
        scenePass.end();
        EXPECT_EXCEPTION(otherPass.end());
    }
    CATCH

    EXPECT_EXCEPTION(paz::Window::ReadPixels())

    try
    {
        paz::Window::EndFrame();
    }
    CATCH

    try
    {
        const paz::Image img = paz::Window::ReadPixels();
        for(const auto& n : SamplePoints)
        {
            if(std::abs(img.bytes()[4*(ImgRes*n[0] + n[1])] - n[2]) > Threshold)
            {
                throw std::runtime_error("Incorrect pixel value at (" + std::
                    to_string(n[0]) + ", " + std::to_string(n[1]) + ").");
            }
        }
    }
    CATCH
}
