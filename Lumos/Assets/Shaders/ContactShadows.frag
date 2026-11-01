#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 Projection;
    mat4 InvProjection;
    vec4 LightDirection; // view space, towards the light
    vec4 Params;         // x = ray length (view units), y = thickness, z = steps, w = frame index (noise)
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Depth;

layout(location = 0) out vec4 outFrag;

vec3 ReconstructVS(vec2 uv, float depth)
{
    vec4 vp = ubo.InvProjection * vec4(uv * 2.0 - 1.0, depth, 1.0);
    return vp.xyz / vp.w;
}

float InterleavedGradientNoise(vec2 p, float frame)
{
    p += frame * 5.588238;
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

// Short screen-space ray towards the sun: catches the contact the shadow map's texels are too big for
void main()
{
    outFrag = vec4(1.0);

    float depth = textureLod(u_Depth, outTexCoord, 0.0).r;
    if(depth <= 0.0 || ubo.LightDirection.w < 0.5) // reversed-Z: 0 = nothing drawn
        return;

    vec3 origin = ReconstructVS(outTexCoord, depth);
    vec3 dir    = normalize(ubo.LightDirection.xyz);

    // longer rays further away, or contacts vanish into a pixel
    float rayLength = ubo.Params.x * (1.0 + -origin.z * 0.02);
    int steps       = int(ubo.Params.z);
    float stepLen   = rayLength / float(steps);
    float thickness = ubo.Params.y * (1.0 + -origin.z * 0.02);
    // depth is coarse far out, and a big flat triangle's depth is not exactly what its
    // neighbour's is: anything less than this behind is the surface itself, not a caster
    float minBehind = 0.01 + 0.004 * -origin.z;

    // start a little off the surface or it shadows itself
    vec3 pos         = origin + dir * stepLen * InterleavedGradientNoise(gl_FragCoord.xy, ubo.Params.w);
    float occlusion  = 0.0;
    for(int i = 0; i < steps; i++)
    {
        pos += dir * stepLen;

        vec4 clip = ubo.Projection * vec4(pos, 1.0);
        if(clip.w <= 0.0)
            break;
        vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
        if(any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
            break;

        float sceneDepth = textureLod(u_Depth, uv, 0.0).r;
        if(sceneDepth <= 0.0)
            continue;

        // > 0: the ray has gone behind what the depth buffer holds there
        float behind = ReconstructVS(uv, sceneDepth).z - pos.z;
        if(behind > minBehind && behind < thickness)
        {
            // fade with distance along the ray and at the screen edge so it never ends in a line
            vec2 edge = smoothstep(0.0, 0.05, uv) * (1.0 - smoothstep(0.95, 1.0, uv));
            occlusion = (1.0 - float(i) / float(steps)) * edge.x * edge.y;
            break;
        }
    }

    outFrag = vec4(1.0 - occlusion, 1.0, 1.0, 1.0);
}
