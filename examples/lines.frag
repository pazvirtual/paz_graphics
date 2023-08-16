uniform sampler2D base;
uniform float width;
in vec2 uv0;
in vec2 uv1;
in vec2 uv2;
in vec2 uv3;
in vec2 uv4;
in vec2 uv5;
in vec2 uv6;
in vec2 uv7;
in vec2 uv8;
layout(location = 0) out vec4 color;
void main()
{
    float d = 1./sqrt(clamp(width, 2., 9.) - 1.);
    color = texture(base, uv0)*clamp(2. - width, 0., 1.);
    color += texture(base, uv1)*d*clamp(width - 1., 0., 2.)*0.5;
    color += texture(base, uv2)*d*clamp(width - 1., 0., 2.)*0.5;
    color += texture(base, uv3)*d*clamp(width - 3., 0., 2.)*0.5;
    color += texture(base, uv4)*d*clamp(width - 3., 0., 2.)*0.5;
    color += texture(base, uv5)*d*clamp(width - 5., 0., 2.)*0.5;
    color += texture(base, uv6)*d*clamp(width - 5., 0., 2.)*0.5;
    color += texture(base, uv7)*d*clamp(width - 7., 0., 2.)*0.5;
    color += texture(base, uv8)*d*clamp(width - 7., 0., 2.)*0.5;
}
