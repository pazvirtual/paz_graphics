in vec2 uv;

layout(location = 0) out vec4 color;

uniform float distSq;

void main()
{
    float d = length(2.*uv - 1.);
    float t = 0.3*max(0., 1. - smoothstep(0., 1., d));
    color = vec4(t*vec3(1., 0.16, 0.01), 1.);
}
