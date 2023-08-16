uniform sampler2D base;
uniform float width;
in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    ivec2 texSize = textureSize(base, 0);
    vec2 texOffset = vec2(1./texSize.x, 1./texSize.y);
    int halfRange = int(0.5*width);
    color = vec4(0.);
    for(int x = -halfRange; x <= halfRange; ++x)
    {
        for(int y = -halfRange; y <= halfRange; ++y)
        {
            float dist = sqrt(x*x + y*y);
            vec4 col = texture(base, uv + texOffset*vec2(x, y));
            color += col*clamp(0.5*width + 0.5 - dist, 0., 1.);
        }
    }
    color /= width;
    color.rgb = pow(color.rgb, vec3(0.4545));
}
