in vec2 uv;
uniform sampler2D img;
layout(location = 0) out vec4 color;
void main()
{
    color = texture(img, uv);
    color.rgb = pow(color.rgb, vec3(0.4545));
}
