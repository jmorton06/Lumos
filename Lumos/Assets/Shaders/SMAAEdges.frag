#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outEdges;

layout(set = 0, binding = 1) uniform sampler2D u_Colour;

const float SMAA_THRESHOLD                          = 0.05;
const float SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR   = 2.0;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

void main()
{
    vec2 rt = 1.0 / vec2(textureSize(u_Colour, 0));

    float L  = luma(texture(u_Colour, uv).rgb);
    float Ll = luma(texture(u_Colour, uv + vec2(-rt.x, 0.0)).rgb);
    float Lt = luma(texture(u_Colour, uv + vec2(0.0, -rt.y)).rgb);

    vec2 delta = abs(vec2(L) - vec2(Ll, Lt));
    vec2 edges = step(vec2(SMAA_THRESHOLD), delta);

    if(dot(edges, vec2(1.0)) == 0.0)
        discard;

    float Lr = luma(texture(u_Colour, uv + vec2(rt.x, 0.0)).rgb);
    float Lb = luma(texture(u_Colour, uv + vec2(0.0, rt.y)).rgb);
    vec2 deltaRB = abs(vec2(L) - vec2(Lr, Lb));
    vec2 maxDelta = max(delta, deltaRB);

    float Lll = luma(texture(u_Colour, uv + vec2(-2.0 * rt.x, 0.0)).rgb);
    float Ltt = luma(texture(u_Colour, uv + vec2(0.0, -2.0 * rt.y)).rgb);
    vec2 deltaFar = abs(vec2(Ll, Lt) - vec2(Lll, Ltt));
    maxDelta = max(maxDelta, deltaFar);

    float finalDelta = max(maxDelta.x, maxDelta.y);

    // Local contrast adaptation: suppress edges next to much stronger ones.
    edges *= step(vec2(finalDelta), SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta);

    outEdges = vec4(edges, 0.0, 1.0);
}
