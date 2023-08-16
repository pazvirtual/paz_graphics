uniform sampler2D render;
uniform sampler2D overlay;

in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    float x = texture(overlay, uv).r;
    color = vec4(x + (1. - x)*texture(render, uv).rgb, 1.);
}
