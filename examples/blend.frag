#version 410 core

uniform sampler2D render;
uniform sampler2D overlay;

in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(texture(render, uv).rgb + texture(overlay, uv).r, 1.);
}
