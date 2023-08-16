const int numChars = 42;
const float scale = 0.005;

uniform int baseSize;
uniform float aspectRatio;
uniform int character;
uniform int col;
uniform int row;

layout(location = 0) in vec2 position;

out vec2 uv;

void main()
{
    uv = 0.5*position.xy + 0.5;
    gl_Position = vec4((uv.x + float(col))*scale*float(baseSize)*2. - 1., (uv.y
        - 1. - 1.5*float(row - 2))*scale*float(baseSize)*2.*aspectRatio, 0.,
        1.);
    uv.x = (uv.x + float(character))/float(numChars);
}
