#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 Reproj;             // current NDC -> previous clip
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
    vec4 here   = texture(u_Texture, outTexCoord);
    float depth = texture(u_Depth, outTexCoord).r;
    depth = min(depth, 0.99999);

    vec4 ndc      = vec4(outTexCoord * 2.0 - 1.0, depth, 1.0);
    vec4 prevClip = ubo.Reproj * ndc;
    // Behind the previous camera: reprojection is meaningless - don't smear.
    if(prevClip.w <= 1e-6)
    {
        outFrag = here;
        return;
    }
    vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;

    vec2 velocity = (outTexCoord - prevUV) * ubo.Strength;
    float vlen    = length(velocity);
    // Static pixel: skip the sample loop entirely.
    if(vlen < 5e-4)
    {
        outFrag = here;
        return;
    }
    const float MAX_VEL = 0.12;
    if(vlen > MAX_VEL)
        velocity *= MAX_VEL / vlen;

    int samples = max(ubo.SampleCount, 1);
    vec4 accum  = vec4(0.0);
    float invN  = 1.0 / float(samples);
    for(int i = 0; i < samples; ++i)
    {
        // Centre the sample run on the current pixel (-0.5..0.5).
        float t = (float(i) + 0.5) * invN - 0.5;
        vec2 uv = outTexCoord + velocity * t;
        // Clamp so off-screen reads don't fetch garbage.
        uv     = clamp(uv, vec2(0.001), vec2(0.999));
        accum += texture(u_Texture, uv);
    }
    outFrag = accum * invN;
}
