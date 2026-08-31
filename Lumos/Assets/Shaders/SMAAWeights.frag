#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outWeights;

layout(set = 0, binding = 1) uniform sampler2D u_Edges;

const int SMAA_MAX_SEARCH_STEPS = 16;

vec2 g_rt;

float searchXLeft(vec2 tc)
{
    float i = 0.0;
    for(int s = 0; s < SMAA_MAX_SEARCH_STEPS; s++)
    {
        vec2 e = texture(u_Edges, tc + vec2(-(i + 1.0), 0.0) * g_rt).rg;
        if(e.g < 0.9 || e.r > 0.0) break;
        i += 1.0;
    }
    return i;
}
float searchXRight(vec2 tc)
{
    float i = 0.0;
    for(int s = 0; s < SMAA_MAX_SEARCH_STEPS; s++)
    {
        vec2 e = texture(u_Edges, tc + vec2((i + 1.0), 0.0) * g_rt).rg;
        if(e.g < 0.9 || e.r > 0.0) break;
        i += 1.0;
    }
    return i;
}
float searchYUp(vec2 tc)
{
    float i = 0.0;
    for(int s = 0; s < SMAA_MAX_SEARCH_STEPS; s++)
    {
        vec2 e = texture(u_Edges, tc + vec2(0.0, -(i + 1.0)) * g_rt).rg;
        if(e.r < 0.9 || e.g > 0.0) break;
        i += 1.0;
    }
    return i;
}
float searchYDown(vec2 tc)
{
    float i = 0.0;
    for(int s = 0; s < SMAA_MAX_SEARCH_STEPS; s++)
    {
        vec2 e = texture(u_Edges, tc + vec2(0.0, (i + 1.0)) * g_rt).rg;
        if(e.r < 0.9 || e.g > 0.0) break;
        i += 1.0;
    }
    return i;
}

float coverage(float d1, float d2, float c1, float c2)
{
    float n = d1 + d2 + 1.0;
    float left  = c1 * max(0.5 * (1.0 - (d1 + 0.5) / n), 0.0);
    float right = c2 * max(0.5 * (1.0 - (d2 + 0.5) / n), 0.0);
    return max(left, right);
}

void main()
{
    g_rt = 1.0 / vec2(textureSize(u_Edges, 0));
    vec4 weights = vec4(0.0);

    vec2 e = texture(u_Edges, uv).rg;

    if(e.g > 0.0) // top edge -> horizontal run, vertical blend
    {
        float dL = searchXLeft(uv);
        float dR = searchXRight(uv);
        float c1 = step(0.9, texture(u_Edges, uv + vec2(-(dL + 1.0), 0.0) * g_rt).r);
        float c2 = step(0.9, texture(u_Edges, uv + vec2((dR + 1.0), 0.0) * g_rt).r);
        weights.r = coverage(dL, dR, c1, c2);
    }

    if(e.r > 0.0) // left edge -> vertical run, horizontal blend
    {
        float dU = searchYUp(uv);
        float dD = searchYDown(uv);
        float c1 = step(0.9, texture(u_Edges, uv + vec2(0.0, -(dU + 1.0)) * g_rt).g);
        float c2 = step(0.9, texture(u_Edges, uv + vec2(0.0, (dD + 1.0)) * g_rt).g);
        weights.g = coverage(dU, dD, c1, c2);
    }

    outWeights = weights;
}
