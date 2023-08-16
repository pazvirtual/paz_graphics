uniform sampler2D font;

in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    float c = texture(font, uv).x;
    color = vec4(c, c, c, 1.);
}
