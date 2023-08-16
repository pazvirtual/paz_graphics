uniform sampler2D hdrRender;
uniform float whitePoint;

in vec2 uv;

layout(location = 0) out vec4 color;

float luminance(in vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 reinhard(in vec3 col)
{
    float lOld = luminance(col);
    float lNew = lOld*(1. + lOld/(whitePoint*whitePoint))/(1. + lOld);
    return col*lNew/lOld;
}

void main()
{
    vec3 hdrCol = texture(hdrRender, uv).rgb;
    color = vec4(pow(reinhard(hdrCol), vec3(0.4545)), 1.);
}
