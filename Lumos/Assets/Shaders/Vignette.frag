#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Radial darkening with adjustable smoothness, roundness, and tint colour.
// Runs after tonemap (LDR) so the falloff reads naturally on the final image.

layout(location = 0) in vec2 outTexCoord;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;
layout(location = 0) out vec4 outFrag;

layout(push_constant) uniform PushConsts
{
    vec3 Colour;
    float Intensity;
    float Smoothness;
    float Roundness;
    float _p0;
    float _p1;
} u;

void main()
{
    vec4 colour = texture(u_Texture, outTexCoord);

    // Distance from centre, scaled by aspect so the falloff isn't oval on
    // wide screens. Roundness lets the user push back toward an oval if
    // they want a more "lens-like" shape.
    vec2 d = (outTexCoord - 0.5) * vec2(u.Roundness, 1.0);
    float r = length(d);

    // smoothstep gives an S-curve falloff: inner radius fully visible,
    // outer radius fully tinted. Smoothness widens the transition band.
    float t = smoothstep(0.5 - u.Smoothness * 0.5, 0.5, r);
    float vignette = 1.0 - t * u.Intensity;

    colour.rgb = mix(u.Colour, colour.rgb, vignette);
    outFrag = vec4(colour.rgb, colour.a);
}
