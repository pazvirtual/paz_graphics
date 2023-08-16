#version 410 core

uniform float aspectRatio;
uniform int character;
uniform int col;
uniform int row;

layout(location = 0) in vec2 position;

out vec2 uv;

void main()
{
    float x = position.x/37. - 1. + 1./37.;
    float y = position.y;
    gl_Position = vec4(x + 2.*col/37., (y/37. - 0.1*row)*aspectRatio, 0., 1.);
    uv = 0.5*(vec2(x + 2.*character/37., y) + 1.);
}
