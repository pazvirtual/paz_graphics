in vec2 uv;

layout(location = 0) out vec4 color;

uniform float distSq;

void main()
{
    float t = 0.3*max(0., 1. - length(2*uv - 1));
    color = vec4(t*vec3(1., 0.16, 0.01), 1.);
}
