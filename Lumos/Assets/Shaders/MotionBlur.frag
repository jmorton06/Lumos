#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Camera-velocity motion blur. Reconstructs each pixel's world position
// from depth + current inverse view-proj, projects it through the previous
// frame's view-proj to get a per-pixel UV velocity, then averages N samples
// along that motion vector. No per-object velocity buffer — purely camera
// motion, so spinning objects don't streak but pans and turns do.

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 InvViewProj;        // current camera
    mat4 PrevViewProj;       // last frame's camera
    float Strength;          // 0..2 typical
    int SampleCount;         // clamped on CPU side
    float _p0;
    float _p1;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(set = 0, binding = 2) uniform sampler2D u_Depth;
layout(location = 0) out vec4 outFrag;

void main()
{
    float depth = texture(u_Depth, outTexCoord).r;
    // Sky / far-plane fragments: skip blur entirely so the horizon doesn't
    // smear into the world.
    if(depth >= 1.0)
    {
        outFrag = texture(u_Texture, outTexCoord);
        return;
    }

    // Reconstruct world position from screen-space depth.
    vec4 ndc      = vec4(outTexCoord * 2.0 - 1.0, depth, 1.0);
    vec4 worldH   = ubo.InvViewProj * ndc;
    vec3 world    = worldH.xyz / worldH.w;

    // Project into previous frame's clip space.
    vec4 prevClip = ubo.PrevViewProj * vec4(world, 1.0);
    vec2 prevUV   = (prevClip.xy / prevClip.w) * 0.5 + 0.5;

    vec2 velocity = (outTexCoord - prevUV) * ubo.Strength;

    int samples   = max(ubo.SampleCount, 1);
    vec4 accum    = vec4(0.0);
    float invN    = 1.0 / float(samples);
    for(int i = 0; i < samples; ++i)
    {
        // Centre the sample run on the current pixel (-0.5..0.5).
        float t  = (float(i) + 0.5) * invN - 0.5;
        vec2 uv  = outTexCoord + velocity * t;
        // Clamp so off-screen reads don't fetch garbage.
        uv       = clamp(uv, vec2(0.001), vec2(0.999));
        accum   += texture(u_Texture, uv);
    }
    outFrag = accum * invN;
}
