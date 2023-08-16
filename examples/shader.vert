uniform float angle;
uniform float aspectRatio;
uniform vec2 origin;
uniform float length;

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

out vec4 c;

void main()
{
    mat2 stretch = mat2(length, 0.,
                            0., 1.);
    mat2 rot = mat2( cos(angle), sin(angle),
                    -sin(angle), cos(angle));
    vec2 pos = mul(rot, mul(stretch, position));
    pos += origin;
    pos.x /= aspectRatio;
    gl_Position = vec4(pos, 0., 1.);
    c = color;
}
