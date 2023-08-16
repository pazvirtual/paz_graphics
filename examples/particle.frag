in vec2 uv;

layout(location = 0) out vec4 color;

uniform float size;
uniform float distSq;

void main()
{
    float a = 0.3*max(0., 1. - length(2*uv - 1) ); // /clamp(size, 0.01, 1.));
    color = vec4(vec3(1., 0.4, 0.1), a);
}
