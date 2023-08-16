in vec2 uv;

layout(location = 0) out vec4 color;

uniform float distSq;
uniform vec3 c;

void main()
{
    float d = length(2.*uv - 1.);
    float t = max(0., 1. - smoothstep(0., 1., d));
    color = vec4(c, t);
}
