uniform sampler2D source;
uniform float factor;
uniform float aspectRatio;

in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    vec3 col = texture(source, uv).rgb;
    for(uint i = 0; i < 10; ++i)
    {
        float d = factor*5e-4*aspectRatio*float(i + 1);
        vec2 u0 = vec2(uv.x, uv.y + d);
        vec2 u1 = vec2(uv.x, uv.y - d);
        vec3 c = mix(texture(source, u0).rgb, texture(source, u1).rgb, 0.5);
        col = mix(col, c, 0.1);
    }
    float d = length(uv - vec2(0.5))*1.41421;
    col *= vec3(1. - d*d);
    col = pow(col, vec3(0.4545));
    color = vec4(col, 1.);
}
