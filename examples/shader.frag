#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

uniform vec2 p;

in vec4 c;

layout(location = 0) out vec4 color;

void main()
{
    color = c;
}
