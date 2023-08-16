uniform sampler2D base;
uniform float width;
in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    ivec2 texSize = textureSize(base, 0);
    vec2 texOffset = vec2(1./texSize.x, 1./texSize.y);
    int halfRange = int(ceil(0.5*width));
    float minDistSq = 1000.;
    color = vec4(0.);
    for(int x = -halfRange; x <= halfRange; ++x)
    {
        for(int y = -halfRange; y <= halfRange; ++y)
        {
            float distSq = x*x + y*y;
            vec4 col = texture(base, uv + texOffset*vec2(x, y));
            if(max(col.r, max(col.b, col.g)) > 0. && distSq < minDistSq)
            {
                minDistSq = distSq;
                color = col;
            }
        }
    }
    float minDist = sqrt(minDistSq);
    color *= clamp(0.5*width + 0.5 - minDist, 0., 1.);
    color.rgb = pow(color.rgb, vec3(0.4545));
}
