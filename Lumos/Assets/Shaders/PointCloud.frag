#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColour;

layout(location = 0) out vec4 outColour;

void main()
{
    // Soft round sprite: radial falloff from centre.
    vec2 d  = vUV * 2.0 - 1.0;
    float r2 = dot(d, d);
    if(r2 > 1.0)
        discard;

    float edge = 1.0 - r2;
    edge = edge * edge;                    // soft outer falloff
    float core = smoothstep(0.35, 0.0, r2); // bright centre plateau
    float a = edge * 0.7 + core;           // combine glow + core
    // Brightness is already baked into vColour.rgb by the vertex stage.
    outColour = vec4(vColour.rgb, 1.0) * (a * 1.35);
}
