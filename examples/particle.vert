layout(location = 0) in vec2 position;

out vec2 uv;

uniform mat4 view;
uniform mat4 projection;

uniform vec3 origin;

void main()
{
    vec4 relOrigin = view*vec4(origin, 1.);
    vec3 pos = relOrigin.xyz + vec3(position, 0.);
    gl_Position = projection*vec4(pos, 1.);
    uv = 0.5*position + 0.5;
}
