uniform sampler2D base;
uniform int width;
in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    ivec2 texSize = textureSize(base, 0);
    vec2 texOffset = vec2(1./texSize.x, 1./texSize.y);
    float maxDist = 0.5*width + 0.5;
    int halfRange = int(maxDist);
    color = vec4(0.);
    for(int i = -halfRange - 1; i <= halfRange; ++i)
    {
        for(int j = -halfRange - 1; j <= halfRange; ++j)
        {
            vec2 offset = vec2(i + 0.5, j + 0.5);
            if(length(offset) < maxDist)
            {
                color += texture(base, uv + texOffset*offset);
            }
        }
    }
    color *= 0.5/halfRange;
    color.rgb = pow(color.rgb, vec3(0.4545));
}
