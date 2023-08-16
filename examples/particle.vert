layout(location = 0) in vec2 position;
layout(location = 1) in vec4 origin [[instance]];
layout(location = 2) in vec4 color [[instance]];

out vec2 uv;
flat out vec3 c;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 relOrigin = mul(view, origin);
    vec3 pos = relOrigin.xyz + vec3(position, 0.);
    gl_Position = mul(projection, vec4(pos, 1.));
    uv = 0.5*position + 0.5;
    c = color.rgb;
}
