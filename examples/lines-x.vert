layout(location = 0) in vec2 pos;
uniform vec2 texOffset;
out vec2 uv0;
out vec2 uv1;
out vec2 uv2;
out vec2 uv3;
out vec2 uv4;
out vec2 uv5;
out vec2 uv6;
out vec2 uv7;
out vec2 uv8;
void main()
{
    gl_Position = vec4(pos, 0., 1.);
    uv0 = 0.5*pos + 0.5;
    uv1 = uv0 + vec2(0.5, 0.)*texOffset;
    uv2 = uv0 - vec2(0.5, 0.)*texOffset;
    uv3 = uv0 + vec2(1.5, 0.)*texOffset;
    uv4 = uv0 - vec2(1.5, 0.)*texOffset;
    uv5 = uv0 + vec2(2.5, 0.)*texOffset;
    uv6 = uv0 - vec2(2.5, 0.)*texOffset;
    uv7 = uv0 + vec2(3.5, 0.)*texOffset;
    uv8 = uv0 - vec2(3.5, 0.)*texOffset;
}
