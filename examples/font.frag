uniform sampler2D font;

in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(vec3(1.), texture(font, uv).x);
}
