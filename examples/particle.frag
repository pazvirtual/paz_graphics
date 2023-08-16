in vec2 uv;
in vec3 c;

layout(location = 0) out vec4 color;

void main()
{
    float d = length(2.*uv - 1.);
    float t = max(0., 1. - smoothstep(0., 1., d));
    color = vec4(c, t);
}
